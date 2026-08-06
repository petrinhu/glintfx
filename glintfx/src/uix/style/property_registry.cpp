// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- implementation. See property_registry.hpp's own header comment for the
//     full scope/boundary/fail-high rationale this file holds itself to.
//
//     THE 72-VS-64-NAMES RECONCILIATION this item's own brief asked to be closed (the open
//     question `docs/uix-rcss.md`'s own `UIX-RCSS-SPEC` TODO.md entry left unresolved: "a
//     reconciliação exata... fica registrada como discrepância aberta na seção 6, não resolvida
//     byte a byte"). Counted by hand against `/var/tmp/censo-rcss-qa1/censo.md` section 3's own
//     64-name list (also hardcoded, separately, in
//     tests/uix_style/property_registry_sanity.cpp's own `test_every_measured_property_is_covered`
//     -- that test is the LIVE, machine-checked version of this same accounting; this comment is
//     the prose narrative of the identical result):
//       - 51 of the 64 census names are direct entries in this file's own 72-row table.
//       - 11 of the 64 census names are SHORTHAND names (`background`, `border`, `border-bottom`,
//         `border-color`, `border-radius`, `border-top`, `flex`, `gap`, `margin`, `overflow`,
//         `padding`) -- not their own registry row (section 6.2's own clause: "um shorthand como
//         `margin` não é ele próprio uma entrada de registro"), but every one of their own
//         constituent longhands IS a row here (proven by shorthand.hpp's own expansion tests).
//       - 2 of the 64 census names (`src`, `-rmlui-fallback-face`) are `@font-face`-only
//         (docs/uix-rcss.md section 10) -- NOT element properties, correctly absent from this
//         table.
//       51 + 11(as shorthand names, not rows) + 2(excluded) = 64, closing the census side.
//     The OTHER direction -- this table's own 72 rows minus the 51 measured-by-name directly --
//     leaves 21 rows that exist ONLY because a measured shorthand expands into them:
//       - 12 `border-*` rows (`border-{top,right,bottom,left}-{width,color}` from `border`/
//         `border-color`'s own expansion, `border-{top,right,bottom,left}-{left,right}-radius`
//         from `border-radius`'s own expansion) -- all reachable from the 5 `border*` census
//         shorthand names above.
//       - `row-gap`/`column-gap` (2) -- from `gap`.
//       - `flex-basis`/`flex-grow`/`flex-shrink` (3) -- from `flex`.
//       - `padding-bottom`/`padding-left` (2) -- from `padding` (its OWN siblings `padding-top`/
//         `padding-right` ARE measured directly by name in the census, at 17 and 8 instances
//         respectively -- `padding-bottom`/`-left` simply never happen to be written standalone
//         anywhere in the corpus, only ever through the `padding` shorthand).
//       12 + 2 + 3 + 2 = 19 of the 21 explained.
//     🔴 THE REMAINING 2 -- `max-height`/`max-width` -- ARE NOT EXPLAINED BY ANY MEASURED NAME OR
//     SHORTHAND. Verified twice: (a) `grep -n "max-height\|max-width"` against
//     `/var/tmp/censo-rcss-qa1/censo.md` in full (not just section 3) returns zero lines; (b)
//     `grep -rn "max-height\|max-width"` against every real corpus source this repo owns
//     (`glintfx/tests/`, `glintfx/demos/`, `glintfx/src/ua_stylesheet.hpp`) also returns zero.
//     Neither name is reachable through ANY of the 13 registered shorthands either (no shorthand
//     in section 6.2 expands into `max-height`/`max-width`). This is a THIRD, genuinely NEW
//     ambiguity beyond the two `UIX-RCSS-SPEC` entry already flagged (comment-mid-run and
//     at-rule-dispatch-layer, both cited in lexer.hpp) -- reported here, not silently
//     "corrected" by dropping the two rows: `docs/uix-rcss.md` section 6.1's own table IS this
//     item's own contract ("implemente o que ela diz, não o que você sabe de CSS" -- this item's
//     own brief, verbatim), so both rows are implemented exactly as written. The likeliest
//     explanation (not verified against a second source, stated as a hypothesis, not a fact): the
//     spec's own author built the 72-row table by hand-transcribing RmlUi's FULL upstream
//     property list for the `min-*`/`max-*` FAMILY once `min-height`/`min-width` were confirmed
//     measured, rather than re-checking each sibling's own census count independently -- but this
//     is a guess, not a claim, and this file does not silently "fix" it either way. See this
//     item's own delivery notes (its own commit message, and the orchestrating agent's own final
//     report) for the full accounting restated for the líder.
// PT: UIX-PROP-REGISTRY -- implementação. Ver o próprio comentário de cabeçalho do
//     property_registry.hpp pro escopo/fronteira/racional-fail-high completos a que este arquivo
//     se prende.
//
//     A RECONCILIAÇÃO 72-VS-64-NOMES que o próprio briefing deste item pediu pra fechar (a
//     pergunta em aberto que a própria entrada `UIX-RCSS-SPEC` do TODO.md do docs/uix-rcss.md
//     deixou sem resolver: "a reconciliação exata... fica registrada como discrepância aberta na
//     seção 6, não resolvida byte a byte"). Contada à mão contra a própria lista de 64 nomes da
//     seção 3 do `/var/tmp/censo-rcss-qa1/censo.md` (também hardcoded, à parte, no próprio
//     `test_every_measured_property_is_covered` do
//     tests/uix_style/property_registry_sanity.cpp -- aquele teste é a versão VIVA, checada por
//     máquina, deste mesmo raciocínio; este comentário é a narrativa em prosa do resultado
//     idêntico):
//       - 51 dos 64 nomes do censo são entradas diretas na própria tabela de 72 linhas deste
//         arquivo.
//       - 11 dos 64 nomes do censo são nomes de SHORTHAND (`background`, `border`,
//         `border-bottom`, `border-color`, `border-radius`, `border-top`, `flex`, `gap`,
//         `margin`, `overflow`, `padding`) -- não a própria linha de registro (a própria cláusula
//         da seção 6.2: "um shorthand como `margin` não é ele próprio uma entrada de registro"),
//         mas cada um dos próprios longhands constituintes deles É uma linha aqui (provado pelos
//         próprios testes de expansão do shorthand.hpp).
//       - 2 dos 64 nomes do censo (`src`, `-rmlui-fallback-face`) são só-de-`@font-face` (seção 10
//         do docs/uix-rcss.md) -- NÃO são propriedades de elemento, corretamente ausentes desta
//         tabela.
//       51 + 11(como nomes de shorthand, não linhas) + 2(excluídos) = 64, fechando o lado do
//       censo.
//     A OUTRA direção -- as próprias 72 linhas desta tabela menos as 51 medidas-por-nome direto --
//     deixa 21 linhas que existem SÓ porque um shorthand medido expande pra elas:
//       - 12 linhas `border-*` (`border-{top,right,bottom,left}-{width,color}` da própria
//         expansão de `border`/`border-color`, `border-{top,right,bottom,left}-{left,right}-radius`
//         da própria expansão de `border-radius`) -- todas alcançáveis pelos 5 nomes de shorthand
//         `border*` do censo acima.
//       - `row-gap`/`column-gap` (2) -- de `gap`.
//       - `flex-basis`/`flex-grow`/`flex-shrink` (3) -- de `flex`.
//       - `padding-bottom`/`padding-left` (2) -- de `padding` (os PRÓPRIOS irmãos
//         `padding-top`/`padding-right` SÃO medidos diretamente por nome no censo, em 17 e 8
//         instâncias respectivamente -- `padding-bottom`/`-left` simplesmente nunca calham de ser
//         escritos sozinhos em lugar nenhum do corpus, só sempre pelo shorthand `padding`).
//       12 + 2 + 3 + 2 = 19 dos 21 explicados.
//     🔴 OS 2 RESTANTES -- `max-height`/`max-width` -- NÃO SÃO EXPLICADOS POR NENHUM NOME OU
//     SHORTHAND MEDIDO. Verificado duas vezes: (a) `grep -n "max-height\|max-width"` contra o
//     `/var/tmp/censo-rcss-qa1/censo.md` inteiro (não só a seção 3) retorna zero linhas; (b)
//     `grep -rn "max-height\|max-width"` contra toda fonte de corpus real que este repo possui
//     (`glintfx/tests/`, `glintfx/demos/`, `glintfx/src/ua_stylesheet.hpp`) também retorna zero.
//     Nenhum dos dois nomes é alcançável por NENHUM dos 13 shorthands registrados também (nenhum
//     shorthand da seção 6.2 expande pra `max-height`/`max-width`). Esta é uma TERCEIRA
//     ambiguidade genuinamente NOVA além das duas que a própria entrada `UIX-RCSS-SPEC` já
//     sinalizava (comentário-no-meio-do-trecho e camada-de-dispatch-de-at-rule, as duas citadas no
//     lexer.hpp) -- reportada aqui, não "consertada" em silêncio derrubando as duas linhas: a
//     própria tabela da seção 6.1 do `docs/uix-rcss.md` É o próprio contrato deste item
//     ("implemente o que ela diz, não o que você sabe de CSS" -- o próprio briefing deste item,
//     verbatim), então as duas linhas são implementadas exatamente como escritas. A explicação
//     mais provável (não verificada contra uma segunda fonte, dita como hipótese, não fato): o
//     próprio autor da spec construiu a tabela de 72 linhas transcrevendo à mão a lista COMPLETA
//     de propriedade upstream do RmlUi pra FAMÍLIA `min-*`/`max-*` assim que `min-height`/
//     `min-width` foram confirmados medidos, em vez de reconferir a própria contagem do censo de
//     cada irmão independentemente -- mas isto é um chute, não uma alegação, e este arquivo não
//     "conserta" isso em silêncio de nenhum dos dois jeitos. Ver as próprias notas de entrega
//     deste item (a própria mensagem de commit dele, e o próprio relatório final do agente
//     orquestrador) pra contagem completa restatada pro líder.
// Copyright (c) 2026 Petrus Silva Costa
#include "property_registry.hpp"

#include <algorithm>

namespace glintfx::uix::style {

namespace {

// EN: docs/uix-rcss.md section 6.1's own table, restated verbatim, alphabetical (this file's own
//     header cites the evidentiary source once; not repeated per row here, same "cite once, not
//     per row" discipline that document's own section 6 states for itself). `LP` below is a local
//     alias for `LengthPercent`, used only to keep the two-domain rows readable in a fixed-width
//     table -- not exported, not a public name.
// PT: A própria tabela da seção 6.1 do docs/uix-rcss.md, restatada verbatim, alfabética (o próprio
//     cabeçalho deste arquivo cita a fonte evidenciária uma vez; não repetida por linha aqui,
//     mesma disciplina "cita uma vez, não por linha" que o próprio documento declara pra si mesmo
//     na seção 6). `LP` abaixo é um alias local pra `LengthPercent`, usado só pra manter as linhas
//     de dois domínios legíveis numa tabela de largura fixa -- não exportado, não é um nome
//     público.
constexpr ValueDomain KW = ValueDomain::Keyword;
constexpr ValueDomain NUM = ValueDomain::Number;
constexpr ValueDomain LEN = ValueDomain::Length;
constexpr ValueDomain LP = ValueDomain::LengthPercent;
constexpr ValueDomain COL = ValueDomain::Color;
constexpr ValueDomain STR = ValueDomain::String;
constexpr ValueDomain CMP = ValueDomain::Composite;

// EN: Single-domain row. `inherited` and `initial` read left-to-right exactly like
//     docs/uix-rcss.md section 6.1's own columns.
// PT: Linha de domínio único. `inherited` e `initial` lidos esquerda-pra-direita exatamente como
//     as próprias colunas da seção 6.1 do docs/uix-rcss.md.
constexpr PropertyInfo one(std::string_view name, std::string_view initial, bool inherited,
                           ValueDomain domain) {
  return PropertyInfo{name, initial, inherited, domain, false, ValueDomain::Keyword};
}

// EN: Two-domain row ("X or Y" cells in the spec's own table).
// PT: Linha de dois domínios (células "X ou Y" da própria tabela da spec).
constexpr PropertyInfo two(std::string_view name, std::string_view initial, bool inherited,
                           ValueDomain domain, ValueDomain alt) {
  return PropertyInfo{name, initial, inherited, domain, true, alt};
}

constexpr PropertyInfo kTable[] = {
    one("align-items", "stretch", false, KW),
    one("animation", "none", false, CMP),
    one("backdrop-filter", "", false, CMP),
    one("background-color", "transparent", false, COL),
    one("border-bottom-color", "black", false, COL),
    one("border-bottom-left-radius", "0px", false, LEN),
    one("border-bottom-right-radius", "0px", false, LEN),
    one("border-bottom-width", "0px", false, LEN),
    one("border-left-color", "black", false, COL),
    one("border-left-width", "0px", false, LEN),
    one("border-right-color", "black", false, COL),
    one("border-right-width", "0px", false, LEN),
    one("border-top-color", "black", false, COL),
    one("border-top-left-radius", "0px", false, LEN),
    one("border-top-right-radius", "0px", false, LEN),
    one("border-top-width", "0px", false, LEN),
    two("bottom", "auto", false, KW, LP),
    one("box-shadow", "none", false, CMP),
    one("box-sizing", "content-box", false, KW),
    one("color", "white", true, COL),
    one("column-gap", "0px", false, LEN),
    one("cursor", "", true, STR),
    one("decorator", "", false, CMP),
    one("display", "inline", false, KW),
    one("filter", "", false, CMP),
    two("flex-basis", "auto", false, KW, LP),
    one("flex-grow", "0", false, NUM),
    one("flex-shrink", "1", false, NUM),
    // EN: ⚠️ `inherited: true`, surprising -- see property_registry.hpp's own header, "Fail-high
    //     policy" paragraph's sibling note; docs/uix-rcss.md section 6.1's own ⚠️ flag, confirmed
    //     at the RmlUi call site, not guessed.
    // PT: ⚠️ `inherited: true`, surpreendente -- ver o próprio parágrafo irmão de "Política
    //     fail-high" do cabeçalho do property_registry.hpp; a própria flag ⚠️ da seção 6.1 do
    //     docs/uix-rcss.md, confirmada no próprio call site do RmlUi, não chutada.
    one("focus", "auto", true, KW),
    one("font-family", "", true, STR),
    one("font-size", "12px", true, LEN),
    two("height", "auto", false, KW, LP),
    one("image-tint-color", "white", false, COL),
    one("image-tint-mode", "none", false, KW),
    one("image-tint-threshold", "0.55", false, NUM),
    one("justify-content", "flex-start", false, KW),
    two("left", "auto", false, KW, LP),
    two("letter-spacing", "normal", true, KW, LEN),
    two("line-height", "1.2", true, NUM, LP),
    two("margin-bottom", "0px", false, KW, LP),
    two("margin-left", "0px", false, KW, LP),
    two("margin-right", "0px", false, KW, LP),
    two("margin-top", "0px", false, KW, LP),
    one("mask-image", "", false, CMP),
    two("max-height", "none", false, KW, LP),
    two("max-width", "none", false, KW, LP),
    one("min-height", "0px", false, LP),
    one("min-width", "0px", false, LP),
    // EN: ⚠️ `inherited: true`, NOT how real CSS `opacity` behaves -- see property_registry.hpp's
    //     own header, "Fail-high policy" paragraph's sibling note; docs/uix-rcss.md section 6.1's
    //     own ⚠️ flag.
    // PT: ⚠️ `inherited: true`, NÃO é como o `opacity` do CSS real se comporta -- ver o próprio
    //     parágrafo irmão de "Política fail-high" do cabeçalho do property_registry.hpp; a própria
    //     flag ⚠️ da seção 6.1 do docs/uix-rcss.md.
    one("opacity", "1", true, NUM),
    one("overflow-x", "visible", false, KW),
    one("overflow-y", "visible", false, KW),
    one("padding-bottom", "0px", false, LP),
    one("padding-left", "0px", false, LP),
    one("padding-right", "0px", false, LP),
    one("padding-top", "0px", false, LP),
    one("position", "static", false, KW),
    two("right", "auto", false, KW, LP),
    one("ripple-origin-x", "0", false, NUM),
    one("ripple-origin-y", "0", false, NUM),
    one("ripple-phase", "0", false, NUM),
    one("ripple-strength", "0", false, NUM),
    one("ripple-width", "48", false, NUM),
    one("row-gap", "0px", false, LEN),
    one("tab-index", "none", false, KW),
    one("text-align", "left", true, KW),
    two("text-overflow", "clip", false, KW, STR),
    one("text-transform", "none", true, KW),
    two("top", "auto", false, KW, LP),
    one("transform", "none", false, CMP),
    two("vertical-align", "baseline", false, KW, LP),
    one("white-space", "normal", true, KW),
    two("width", "auto", false, KW, LP),
};

constexpr std::size_t kTableSize = sizeof(kTable) / sizeof(kTable[0]);

} // namespace

const PropertyInfo* find_property(std::string_view name) {
  const auto it = std::find_if(std::begin(kTable), std::end(kTable),
                               [name](const PropertyInfo& entry) { return entry.name == name; });
  return it == std::end(kTable) ? nullptr : &*it;
}

std::span<const PropertyInfo> all_properties() {
  return std::span<const PropertyInfo>(kTable, kTableSize);
}

} // namespace glintfx::uix::style
