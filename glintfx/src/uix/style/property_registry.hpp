// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- glintfx's own RCSS property registry: the fixed, closed table this
//     style engine's future cascade slice (RMLX-2) enumerates one entry per registered
//     property, per node, per docs/uix-rcss.md section 3's own `PROPS <n>` contract (`n` is this
//     table's own size). THIRD brick of the style engine, after S1's lexer.hpp/.cpp (tokenizer)
//     and this item's own sibling, shorthand.hpp/.cpp (shorthand-to-longhand expansion, which
//     this file's table is the target of).
//
//     SOURCE OF TRUTH: `docs/uix-rcss.md` section 6.1's own 72-row table (name, initial value,
//     `inherited` flag, value domain), itself cited against
//     `examples/RmlUi/Source/Core/StyleSheetSpecification.cpp:262-433`'s own
//     `RegisterProperty(id, name, default_value, inherited, forces_layout)` call sites (4th
//     positional argument is `inherited`, per that document's own explicit warning against
//     misreading the signature) plus `glintfx/src/rml/decorator_ripple.cpp:332-336` and
//     `glintfx/src/rml/decorator_image_tint.cpp:409-411` for the 5 glintfx-authored custom
//     `ripple-*`/`image-tint-*` entries. This file's own property_registry.cpp restates that
//     table verbatim, once, as a `constexpr` array -- see that file's own header for the
//     72-vs-64-names reconciliation this item's own brief asked for (not duplicated here).
//
//     SCOPE, THIS ITEM'S OWN DECLARED TETO -- what this table IS and is NOT:
//       - IS: name lookup, initial value (for the "author never declared it" case docs/uix-rcss.md
//         section 3 requires every `PROPS` block to still emit), the `inherited` flag, and a
//         `ValueDomain` TAG (section 7's own domain key, `keyword`/`number`/`length`/
//         `length-percent`/`color`/`string`/`composite`) so a future consumer knows WHICH of
//         section 7's canonical print rules applies -- it does NOT implement that print rule
//         itself (quantization, hex formatting, composite-list serialization are section 8/9's
//         own job, a future slice's, not this registry's).
//       - IS NOT: a value parser or validator. `find_property()` never inspects a declaration's
//         VALUE text, only its NAME. A property whose table row lists TWO domains ("keyword(auto)
//         or length-percent") exposes BOTH via `domain`/`alternate_domain` -- this table does not
//         decide, for a given raw value string, WHICH of the two applies (that decision needs the
//         actual value text, parser territory, out of this item's own brief: "não resolva
//         porcentagem... não implemente parse de valor").
//       - IS NOT a percentage resolver -- `ValueDomain::LengthPercent` is a single tag covering
//         all three `%`-families docs/uix-rcss.md section 5 names; this registry does not
//         distinguish which family a given property's `%` belongs to beyond what section 5's own
//         table already fixes per-property (every `LengthPercent`-tagged entry in this table is
//         family (a), box-relative -- families (b)/(c) live inside `decorator`/`mask-image`'s own
//         COMPOSITE grammar, section 9.2, not as their own registry entries).
//       - IS NOT the composite sub-grammar dispatcher -- `ValueDomain::Composite` is one tag for
//         all five composite kinds (`shadow-list`/`decorator-list`/`filter-list`/
//         `transform-list`/`animation`, section 9's own per-kind grammars); this registry only
//         flags "this property's value is a list, not a scalar token", enough for a caller to not
//         mis-treat it as one -- the actual per-kind grammar is a later slice's job (whichever
//         wires up section 9), not this item's ("`decorator`'s 5 sub-linguagens" is explicitly
//         out of scope per this item's own brief).
//
//     FAIL-HIGH POLICY (docs/uix-rcss.md section 11, this item's own DoD): `find_property()`
//     returns `nullptr` for any name not in this closed table -- it never logs (this repo's own
//     zero-logging-inside-a-standalone-module precedent, already established by the DOM sibling's
//     own `dom_tree.cpp` and restated in this module's own lexer.hpp). The CALLER decides what
//     "unknown property" means for its own context (a future cascade slice logs-and-ignores, per
//     the already-canonized `polygon()` precedent this item's own brief cites) -- this registry's
//     own job stops at "is this name known, and if so, what does it mean".
//
//     NAMESPACE: `glintfx::uix::style`, same module as lexer.hpp -- see that file's own header
//     for why this is NOT the DOM sibling's flat `glintfx::uix`.
// PT: UIX-PROP-REGISTRY -- registro de propriedades RCSS próprio da glintfx: a tabela fixa e
//     fechada que a futura fatia de cascata deste motor de estilo (RMLX-2) enumera, uma entrada
//     por propriedade registrada, por nó, per o próprio contrato `PROPS <n>` da seção 3 do
//     docs/uix-rcss.md (`n` é o próprio tamanho desta tabela). TERCEIRO tijolo do motor de estilo,
//     depois do lexer.hpp/.cpp da S1 (tokenizador) e do próprio irmão deste item,
//     shorthand.hpp/.cpp (expansão de shorthand pra longhand, cujo alvo é a própria tabela deste
//     arquivo).
//
//     FONTE DE VERDADE: a própria tabela de 72 linhas da seção 6.1 do `docs/uix-rcss.md` (nome,
//     valor inicial, flag `inherited`, domínio de valor), ela própria citada contra o próprio
//     `examples/RmlUi/Source/Core/StyleSheetSpecification.cpp:262-433` (chamadas
//     `RegisterProperty(id, name, default_value, inherited, forces_layout)`, 4º argumento
//     posicional é `inherited`, per o próprio aviso explícito daquele documento contra má-leitura
//     da assinatura) mais `glintfx/src/rml/decorator_ripple.cpp:332-336` e
//     `glintfx/src/rml/decorator_image_tint.cpp:409-411` pras 5 entradas `ripple-*`/
//     `image-tint-*` custom, autoradas pela glintfx. O próprio property_registry.cpp deste arquivo
//     restata aquela tabela verbatim, uma vez, como um array `constexpr` -- ver o próprio
//     cabeçalho daquele arquivo pra reconciliação 72-vs-64-nomes que o próprio briefing deste item
//     pediu (não duplicada aqui).
//
//     ESCOPO, O PRÓPRIO TETO DECLARADO DESTE ITEM -- o que esta tabela É e NÃO É:
//       - É: busca por nome, valor inicial (pro caso "o autor nunca declarou" que a própria seção
//         3 do docs/uix-rcss.md exige que todo bloco `PROPS` ainda emita), a flag `inherited`, e
//         uma TAG `ValueDomain` (a própria chave de domínio da seção 7, `keyword`/`number`/
//         `length`/`length-percent`/`color`/`string`/`composite`) pra um futuro consumidor saber
//         QUAL das próprias regras de impressão canônica da seção 7 se aplica -- não implementa
//         essa regra de impressão em si (quantização, formatação hex, serialização de lista
//         composta são o próprio trabalho da seção 8/9, de uma futura fatia, não deste registro).
//       - NÃO É: um parser ou validador de valor. `find_property()` nunca inspeciona o texto de
//         VALOR de uma declaração, só o NOME dela. Uma propriedade cuja linha da tabela lista DOIS
//         domínios ("keyword(auto) ou length-percent") expõe OS DOIS via `domain`/
//         `alternate_domain` -- esta tabela não decide, pra um dado texto de valor cru, QUAL dos
//         dois se aplica (essa decisão precisa do próprio texto de valor, território de parser,
//         fora do próprio briefing deste item: "não resolva porcentagem... não implemente parse
//         de valor").
//       - NÃO É um resolvedor de porcentagem -- `ValueDomain::LengthPercent` é uma tag única
//         cobrindo as três famílias de `%` que a própria seção 5 do docs/uix-rcss.md nomeia; este
//         registro não distingue a QUAL família o `%` de uma dada propriedade pertence além do que
//         a própria tabela da seção 5 já fixa por propriedade (toda entrada marcada
//         `LengthPercent` nesta tabela é família (a), box-relativa -- as famílias (b)/(c) moram
//         dentro da própria gramática COMPOSTA de `decorator`/`mask-image`, seção 9.2, não como
//         entradas de registro próprias).
//       - NÃO É o despachante de sub-gramática composta -- `ValueDomain::Composite` é uma tag só
//         pras cinco espécies compostas (`shadow-list`/`decorator-list`/`filter-list`/
//         `transform-list`/`animation`, as próprias gramáticas por-espécie da seção 9); este
//         registro só sinaliza "o valor desta propriedade é uma lista, não um token escalar",
//         suficiente pra um chamador não tratá-la errado como um só -- a própria gramática
//         por-espécie é trabalho de uma fatia posterior (a que ligar a seção 9), não deste item
//         ("as 5 sub-linguagens do `decorator`" está explicitamente fora de escopo pelo próprio
//         briefing deste item).
//
//     POLÍTICA FAIL-HIGH (seção 11 do docs/uix-rcss.md, o próprio DoD deste item):
//     `find_property()` retorna `nullptr` pra todo nome fora desta tabela fechada -- nunca loga
//     (o próprio precedente deste repo de zero-logging dentro de um módulo standalone, já
//     estabelecido pelo próprio `dom_tree.cpp` do irmão DOM e restatado no próprio lexer.hpp deste
//     módulo). O CHAMADOR decide o que "propriedade desconhecida" significa pro próprio contexto
//     dele (uma futura fatia de cascata loga-e-ignora, pelo próprio precedente já canonizado do
//     `polygon()` que o próprio briefing deste item cita) -- o próprio trabalho deste registro
//     para em "este nome é conhecido, e se for, o que ele significa".
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo do lexer.hpp -- ver o próprio cabeçalho
//     daquele arquivo pro porquê disto NÃO ser o `glintfx::uix` plano do irmão DOM.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace glintfx::uix::style {

// EN: See this file's header comment, "Scope" paragraph, for what each tag does and does not
//     imply. Closed set, keyed 1:1 to docs/uix-rcss.md section 7's own domain column -- a new
//     variant needs a new section-7 row first, same "spec is the contract" discipline this item's
//     own brief states.
// PT: Ver o parágrafo "Escopo" do comentário de cabeçalho deste arquivo pro que cada tag implica e
//     não implica. Conjunto fechado, chaveado 1:1 à própria coluna de domínio da seção 7 do
//     docs/uix-rcss.md -- uma variante nova precisa de uma linha nova na seção 7 primeiro, mesma
//     disciplina "a spec é o contrato" que o próprio briefing deste item declara.
enum class ValueDomain {
  Keyword,
  Number,
  Length,
  LengthPercent, // family (a) only -- box-relative, symbolic %, see header "Scope"
  Color,
  String,
  Composite, // shadow-list / decorator-list / filter-list / transform-list / animation -- §9
};

// EN: One row of docs/uix-rcss.md section 6.1's own table. `initial_value` is an empty
//     `string_view` for the table's own `*(empty)*` cells (cursor, font-family, decorator,
//     filter, backdrop-filter, mask-image) -- an EMPTY string IS the initial value there, not "no
//     data" (RmlUi's own default_value argument is a literal `""` at those call sites). When
//     `has_alternate_domain` is true, the property's own table cell listed TWO domains ("X or
//     Y") -- see header "Scope" for why this table does not (and structurally cannot, without a
//     value string) decide which one a given declaration uses.
// PT: Uma linha da própria tabela da seção 6.1 do docs/uix-rcss.md. `initial_value` é um
//     `string_view` vazio pras próprias células `*(empty)*` da tabela (cursor, font-family,
//     decorator, filter, backdrop-filter, mask-image) -- uma string VAZIA É o valor inicial ali,
//     não "sem dado" (o próprio argumento default_value do RmlUi é um `""` literal nesses call
//     sites). Quando `has_alternate_domain` é verdadeiro, a própria célula de domínio da
//     propriedade listava DOIS domínios ("X ou Y") -- ver "Escopo" no cabeçalho pro porquê desta
//     tabela não decidir (e estruturalmente não conseguir, sem um texto de valor) qual dos dois
//     uma dada declaração usa.
struct PropertyInfo {
  std::string_view name;
  std::string_view initial_value;
  bool inherited = false;
  ValueDomain domain = ValueDomain::Keyword;
  bool has_alternate_domain = false;
  ValueDomain alternate_domain = ValueDomain::Keyword;
};

// EN: Linear scan over a 72-entry table -- deliberately NOT a hash map or `std::lower_bound`
//     despite the table being sorted (property_registry.cpp's own static_assert-adjacent test
//     coverage proves the sort, see tests/uix_style/property_registry_sanity.cpp): this is not a
//     per-frame hot path at this item's own stage (no cascade/parser consumes it yet), and a
//     linear scan over 72 short `string_view` comparisons is not a measured bottleneck anywhere
//     in this repo -- a future consumer that DOES put this on a hot path can switch to
//     `std::lower_bound` against `all_properties()` (already sorted) without changing this
//     function's own contract. Returns `nullptr` for any name outside the closed table -- see
//     header "Fail-high policy".
// PT: Varredura linear sobre uma tabela de 72 entradas -- deliberadamente NÃO um hash map nem
//     `std::lower_bound` apesar da tabela estar ordenada (a própria cobertura de teste adjacente
//     a static_assert do property_registry.cpp prova a ordenação, ver o próprio
//     tests/uix_style/property_registry_sanity.cpp): isto não é um caminho quente por-frame no
//     próprio estágio deste item (nenhuma cascata/parser consome isto ainda), e uma varredura
//     linear sobre 72 comparações curtas de `string_view` não é um gargalo medido em lugar nenhum
//     deste repo -- um futuro consumidor que PUSER isto num caminho quente pode trocar pra
//     `std::lower_bound` contra `all_properties()` (já ordenada) sem mudar o próprio contrato
//     desta função. Retorna `nullptr` pra todo nome fora da tabela fechada -- ver "Política
//     fail-high" no cabeçalho.
const PropertyInfo* find_property(std::string_view name);

// EN: The whole table, in the exact order property_registry.cpp declares it (alphabetical,
//     byte-wise, matching docs/uix-rcss.md section 6's own required `PROP`-line sort order) --
//     for a future consumer that needs to enumerate "every registry entry per node" per
//     docs/uix-rcss.md section 3's own dump contract, and for this item's own tests.
// PT: A tabela inteira, na ordem exata em que o property_registry.cpp a declara (alfabética,
//     byte-a-byte, casando com a própria ordem de sort exigida pra linha `PROP` da seção 6 do
//     docs/uix-rcss.md) -- pra um futuro consumidor que precisar enumerar "todo registro por nó"
//     per o próprio contrato de dump da seção 3 do docs/uix-rcss.md, e pros próprios testes deste
//     item.
std::span<const PropertyInfo> all_properties();

} // namespace glintfx::uix::style
