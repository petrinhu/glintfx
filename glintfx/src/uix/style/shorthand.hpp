// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- glintfx's own shorthand-to-longhand expansion: the second half of this
//     item's own brief ("a tabela e a expansão"), the sibling of property_registry.hpp. Given a
//     shorthand property's own NAME and its RAW (already whitespace-stripped-once, e.g. a
//     style::Lexer's own `Declaration.value`) VALUE TEXT, produces the list of
//     `{longhand-name, longhand-raw-value}` pairs docs/uix-rcss.md section 6.2's own table
//     describes -- one of `Box`/`FallThrough`/`RecursiveRepeat`/`Replicate`/`Flex`, cited per-row
//     there against `examples/RmlUi/Source/Core/PropertySpecification.cpp:311-472` (read
//     directly for this item, every algorithm below traces to a specific line range in that
//     function, not a paraphrase of the spec's own prose).
//
//     SCOPE, THIS ITEM'S OWN DECLARED TETO -- same "not a value parser" boundary
//     property_registry.hpp states, restated for the specific shape this file's own algorithms
//     need:
//       - This module tokenizes a shorthand's raw value by WHITESPACE RUNS, with one exception
//         added by `ESC-2`: a double- or single-quoted run collapses to ONE token regardless of
//         internal whitespace (`font`'s own `font-family` item is the one target, among the 20,
//         whose accepted grammar is a string that may contain a space, e.g. `"Times New Roman"`
//         -- see `split_whitespace`'s own doc-comment in shorthand.cpp for the exact
//         boundary/escape contract). This module still does NOT understand nested parentheses or
//         comma-separated lists, because none of the 20 registered shorthands' own accepted
//         grammars need either (verified against docs/uix-rcss.md section 6.2's own table: every
//         shorthand here is a short, space-separated token run -- the corpus's genuinely rich,
//         nested-paren/comma-list grammars, `box-shadow`/`decorator`, are NOT shorthands at all,
//         they are single COMPOSITE-domain longhand properties, section 9's own territory,
//         untouched by this file).
//       - The `FallThrough`/`Flex` algorithms below DO need to decide, for a given raw token,
//         which of two candidate longhand DOMAINS it looks like it belongs to (e.g. `border-top`'s
//         own value routes each of its 2 tokens to `-width` or `-color`). This is done by a pair
//         of NARROW, SYNTACTIC ROUTING CLASSIFIERS (`looks_like_number_token`/
//         `looks_like_length_token`, this file's own .cpp) -- these decide "which longhand does
//         this token's SHAPE suggest", never "is this token a semantically VALID value for that
//         longhand's own domain" (unit correctness, color-hex digit-count validity, range
//         clamping are explicitly a later parser slice's job, same boundary
//         property_registry.hpp's own header states for `ValueDomain` itself). This is the
//         narrowest implementation of `PropertySpecification.cpp`'s own real `ParseValue`-based
//         FallThrough dispatch this item's own boundary allows -- see shorthand.cpp's own header
//         for the exact classifier definitions and the citation proving upstream's OWN ordering
//         requirement: 🔴 `border-top`'s 2 tokens must arrive width-THEN-color for BOTH to be
//         consumed -- the REVERSED order is `MalformedValue`, a correction to docs/uix-rcss.md
//         section 6.2's own "order-independent between the two" phrase, traced iteration-by-
//         iteration from upstream's own real loop, not guessed (see
//         tests/uix_style/shorthand_expansion_sanity.cpp's own
//         `test_border_top_fallthrough_order_is_load_bearing` for the full trace).
//       - This module does NOT validate that the LONGHAND NAMES it produces exist in
//         property_registry.hpp's own table at runtime (that would be redundant per-call work for
//         a fact fixed at compile time) -- this closure is instead a build-time-adjacent
//         INVARIANT this item's own test suite proves once
//         (tests/uix_style/property_registry_sanity.cpp's own coverage test cross-checks every
//         shorthand's own target names against `find_property()`).
//
//     FAIL-HIGH POLICY (same contract as property_registry.hpp's own header): `expand_shorthand`
//     never logs, never crashes on malformed input -- `ShorthandExpandStatus` names the two
//     failure kinds explicitly (`UnknownShorthand` for a name this table does not recognise,
//     `MalformedValue` for a recognised shorthand whose raw value's own token SHAPE does not fit
//     any of its algorithm's accepted forms), and the caller decides what either means for its own
//     context, same "this module's own job stops at classifying the input" discipline
//     property_registry.hpp's `find_property` already establishes.
//
//     NAMESPACE: `glintfx::uix::style`, same module as lexer.hpp/property_registry.hpp.
// PT: UIX-PROP-REGISTRY -- expansão shorthand-pra-longhand própria da glintfx: a segunda metade do
//     próprio briefing deste item ("a tabela e a expansão"), o irmão do property_registry.hpp.
//     Dado o próprio NOME de uma propriedade shorthand e o próprio TEXTO DE VALOR CRU dela (já
//     whitespace-stripado uma vez, ex. o próprio `Declaration.value` de um style::Lexer), produz a
//     lista de pares `{nome-longhand, valor-cru-longhand}` que a própria tabela da seção 6.2 do
//     docs/uix-rcss.md descreve -- um de `Box`/`FallThrough`/`RecursiveRepeat`/`Replicate`/`Flex`,
//     citado por-linha ali contra o `examples/RmlUi/Source/Core/PropertySpecification.cpp:311-472`
//     (lido direto pra este item, todo algoritmo abaixo remonta a um intervalo de linha específico
//     naquela função, não uma paráfrase da própria prosa da spec).
//
//     ESCOPO, O PRÓPRIO TETO DECLARADO DESTE ITEM -- mesma fronteira "não é um parser de valor"
//     que o property_registry.hpp declara, restatada pra forma específica que os próprios
//     algoritmos deste arquivo precisam:
//       - Este módulo tokeniza o valor cru de um shorthand POR TRECHOS DE WHITESPACE, com uma
//         exceção somada pela `ESC-2`: um trecho entre aspas duplas ou simples colapsa num ÚNICO
//         token independente de whitespace interno (o próprio item `font-family` do `font` é o
//         único alvo, dentre os 20, cuja própria gramática aceita uma string que pode conter
//         espaço, ex. `"Times New Roman"` -- ver o próprio comentário de doc do
//         `split_whitespace` no shorthand.cpp pro contrato exato de fronteira/escape). Este
//         módulo continua NÃO entendendo parênteses aninhados nem listas separadas por vírgula,
//         porque nenhum dos 20 shorthands registrados precisa de nenhum dos dois (verificado
//         contra a própria tabela da seção 6.2 do docs/uix-rcss.md: todo shorthand aqui é um
//         trecho curto, de tokens separados por espaço -- as próprias gramáticas genuinamente
//         ricas, parênteses-aninhados/lista-por-vírgula do corpus, `box-shadow`/`decorator`, NÃO
//         são shorthands de jeito nenhum, são propriedades longhand de domínio COMPOSTO únicas,
//         território da própria seção 9, intocado por este arquivo).
//       - Os próprios algoritmos `FallThrough`/`Flex` abaixo PRECISAM decidir, pra um dado token
//         cru, a qual de dois DOMÍNIOS de longhand candidatos ele parece pertencer (ex. o próprio
//         valor de `border-top` roteia cada um dos 2 tokens dele pra `-width` ou `-color`). Isto é
//         feito por um par de CLASSIFICADORES DE ROTEAMENTO SINTÁTICOS, ESTREITOS
//         (`looks_like_number_token`/`looks_like_length_token`, do próprio .cpp deste arquivo) --
//         eles decidem "qual longhand a FORMA deste token sugere", nunca "este token é um valor
//         semanticamente VÁLIDO pro próprio domínio daquele longhand" (correção de unidade,
//         validade de contagem de dígito hex de cor, clamp de faixa são explicitamente trabalho de
//         uma futura fatia de parser, mesma fronteira que o próprio cabeçalho do
//         property_registry.hpp declara pro próprio `ValueDomain`). Esta é a implementação mais
//         estreita do próprio dispatch de FallThrough baseado em `ParseValue` real do
//         `PropertySpecification.cpp` que a própria fronteira deste item permite -- ver o próprio
//         cabeçalho do shorthand.cpp pras definições exatas dos classificadores e a citação que
//         prova a PRÓPRIA exigência de ordenação do upstream: 🔴 os 2 tokens de `border-top`
//         precisam chegar width-DEPOIS-color pros DOIS serem consumidos -- a ordem REVERTIDA é
//         `MalformedValue`, uma correção à própria frase "independente de ordem entre os dois" da
//         seção 6.2 do docs/uix-rcss.md, rastreada iteração-por-iteração do próprio laço real do
//         upstream, não chutada (ver o próprio
//         `test_border_top_fallthrough_order_is_load_bearing` do
//         tests/uix_style/shorthand_expansion_sanity.cpp pro rastro completo).
//       - Este módulo NÃO valida que os NOMES DE LONGHAND que ele produz existem na própria
//         tabela do property_registry.hpp em tempo de execução (seria trabalho redundante
//         por-chamada pra um fato fixo em tempo de compilação) -- este fechamento é em vez disso
//         um INVARIANTE adjacente-a-tempo-de-build que a própria suíte de teste deste item prova
//         uma vez (o próprio teste de cobertura do
//         tests/uix_style/property_registry_sanity.cpp confere cruzado os próprios nomes-alvo de
//         todo shorthand contra o `find_property()`).
//
//     POLÍTICA FAIL-HIGH (mesmo contrato do próprio cabeçalho do property_registry.hpp):
//     `expand_shorthand` nunca loga, nunca trava com input malformado -- `ShorthandExpandStatus`
//     nomeia os dois tipos de falha explicitamente (`UnknownShorthand` pra um nome que esta tabela
//     não reconhece, `MalformedValue` pra um shorthand reconhecido cujo próprio valor cru tem
//     FORMA de token que não cabe em nenhuma das formas aceitas pelo próprio algoritmo), e o
//     chamador decide o que cada um significa pro próprio contexto dele, mesma disciplina "o
//     próprio trabalho deste módulo para em classificar o input" que o próprio `find_property` do
//     property_registry.hpp já estabelece.
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo do lexer.hpp/property_registry.hpp.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <span>
#include <string_view>
#include <vector>

namespace glintfx::uix::style {

// EN: One `{longhand-name, longhand-raw-value}` pair `expand_shorthand` produces. `value` is a
//     view into the CALLER's own `raw_value` buffer (or, for `Flex`'s own "none"/omitted-value
//     defaults, a view into a literal owned by this module -- both are `std::string_view`, the
//     caller does not need to know which; neither ever dangles past this call because both
//     outlive it: the caller's buffer by contract, the literal by static storage duration).
// PT: Um par `{nome-longhand, valor-cru-longhand}` que o `expand_shorthand` produz. `value` é uma
//     view sobre o próprio buffer `raw_value` do CHAMADOR (ou, pros próprios defaults de
//     "none"/valor-omitido do `Flex`, uma view sobre um literal de posse deste módulo -- os dois
//     são `std::string_view`, o chamador não precisa saber qual; nenhum dos dois nunca fica
//     pendurado além desta chamada porque os dois sobrevivem a ela: o buffer do chamador por
//     contrato, o literal por duração de armazenamento estática).
struct LonghandValue {
  std::string_view name;
  std::string_view value;
};

// EN: See this file's header comment, "Fail-high policy".
// PT: Ver o parágrafo "Política fail-high" do comentário de cabeçalho deste arquivo.
enum class ShorthandExpandStatus {
  Ok,
  UnknownShorthand,
  MalformedValue,
};

// EN: True iff `name` is one of the 20 registered shorthand names (docs/uix-rcss.md section 6.2's
//     own table) -- for a caller that needs to distinguish "this is a shorthand" from "this is a
//     plain longhand" before deciding whether to call `expand_shorthand` at all (e.g. a future
//     cascade slice: try `find_property()` first, and only call this when that returns `nullptr`
//     AND `is_shorthand()` returns true).
// PT: Verdadeiro sse `name` é um dos 20 nomes de shorthand registrados (a própria tabela da seção
//     6.2 do docs/uix-rcss.md) -- pra um chamador que precisa distinguir "isto é um shorthand" de
//     "isto é um longhand comum" antes de decidir se chama o `expand_shorthand` de jeito nenhum
//     (ex. uma futura fatia de cascata: tenta `find_property()` primeiro, e só chama isto quando
//     aquilo retornar `nullptr` E `is_shorthand()` retornar verdadeiro).
bool is_shorthand(std::string_view name);

// EN: Expands `shorthand_name`'s `raw_value` into its longhand constituents, per
//     docs/uix-rcss.md section 6.2's own algorithm table -- clears `*out` first (matching this
//     repo's own "output parameter always starts from a known state" convention), appends on
//     success. See this file's header comment for the exact scope boundary and the two failure
//     kinds.
// PT: Expande o `raw_value` de `shorthand_name` nos próprios constituintes longhand, per a própria
//     tabela de algoritmo da seção 6.2 do docs/uix-rcss.md -- limpa `*out` primeiro (casando com a
//     própria convenção deste repo de "parâmetro de saída sempre começa de um estado conhecido"),
//     acrescenta em caso de sucesso. Ver o comentário de cabeçalho deste arquivo pra fronteira de
//     escopo exata e os dois tipos de falha.
ShorthandExpandStatus expand_shorthand(std::string_view shorthand_name, std::string_view raw_value,
                                       std::vector<LonghandValue>* out);

// EN: One row of the 20-entry shorthand table, exposed for enumeration (this item's own test
//     suite; a future consumer that needs to list every registered shorthand name, e.g. to build
//     an `is_shorthand`-equivalent lookup of its own).
// PT: Uma linha da tabela de 20 shorthands, exposta pra enumeração (a própria suíte de teste deste
//     item; um futuro consumidor que precisar listar todo nome de shorthand registrado, ex. pra
//     construir uma busca própria equivalente ao `is_shorthand`).
struct ShorthandDescriptor {
  std::string_view name;
};

std::span<const ShorthandDescriptor> all_shorthands();

} // namespace glintfx::uix::style
