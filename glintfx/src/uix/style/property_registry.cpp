// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- implementation. See property_registry.hpp's own header comment for the
//     full scope/boundary/fail-high rationale this file holds itself to.
//
//     THE 107-VS-64-NAMES RECONCILIATION, updated by `ESC-1` (2026-08-07, +35 rows, 72 -> 107) --
//     the ORIGINAL 72-vs-64 reconciliation this item's own brief once asked to be closed (the open
//     question `docs/uix-rcss.md`'s own `UIX-RCSS-SPEC` TODO.md entry left unresolved: "a
//     reconciliação exata... fica registrada como discrepância aberta na seção 6, não resolvida
//     byte a byte"). Counted by hand against `/var/tmp/censo-rcss-qa1/censo.md` section 3's own
//     64-name list (also hardcoded, separately, in
//     tests/uix_style/property_registry_sanity.cpp's own `test_every_measured_property_is_covered`
//     -- that test is the LIVE, machine-checked version of this same accounting; this comment is
//     the prose narrative of the identical result). **The census side is FIXED, `ESC-1` did not
//     touch it** -- none of the 35 new rows below collide with a census-measured name or a
//     census-measured shorthand name, verified directly (the new count closes exact, no new open
//     discrepancy):
//       - 51 of the 64 census names are direct entries in this file's own 107-row table
//         (unchanged by `ESC-1` -- none of the 35 new rows share a name with a census entry).
//       - 11 of the 64 census names are SHORTHAND names (`background`, `border`, `border-bottom`,
//         `border-color`, `border-radius`, `border-top`, `flex`, `gap`, `margin`, `overflow`,
//         `padding`) -- not their own registry row (section 6.2's own clause: "um shorthand como
//         `margin` não é ele próprio uma entrada de registro"), but every one of their own
//         constituent longhands IS a row here (proven by shorthand.hpp's own expansion tests).
//       - 2 of the 64 census names (`src`, `-rmlui-fallback-face`) are `@font-face`-only
//         (docs/uix-rcss.md section 10) -- NOT element properties, correctly absent from this
//         table.
//       51 + 11(as shorthand names, not rows) + 2(excluded) = 64, closing the census side.
//     The OTHER direction -- this table's own 107 rows minus the 51 measured-by-name directly --
//     leaves 56 rows with no direct census measurement, now closing exact across THREE buckets
//     (the pre-`ESC-1` version of this comment had only two, plus an unexplained remainder):
//       - 19 rows exist ONLY because a measured shorthand expands into them (unchanged by
//         `ESC-1`): 12 `border-*` rows (`border-{top,right,bottom,left}-{width,color}` from
//         `border`/`border-color`'s own expansion, `border-{top,right,bottom,left}-{left,right}-
//         radius` from `border-radius`'s own expansion) -- all reachable from the 5 `border*`
//         census shorthand names above; `row-gap`/`column-gap` (2) -- from `gap`;
//         `flex-basis`/`flex-grow`/`flex-shrink` (3) -- from `flex`; `padding-bottom`/
//         `padding-left` (2) -- from `padding` (its OWN siblings `padding-top`/`padding-right` ARE
//         measured directly by name in the census, at 17 and 8 instances respectively --
//         `padding-bottom`/`-left` simply never happen to be written standalone anywhere in the
//         corpus, only ever through the `padding` shorthand). 12+2+3+2 = 19.
//       - `max-height`/`max-width` (2) -- RECLASSIFIED by `ESC-1`, not re-explained by a NEW
//         mechanism: this pair used to be "the one registry entry with zero corpus justification",
//         a standalone, unexplained anomaly this file's own earlier revision reported as a genuine
//         open question (verified twice against the real corpus, see this comment's own git
//         history for that original verification narrative, preserved in the commit log rather
//         than repeated here). `ESC-1` does not change that underlying fact -- these two rows are
//         still not measured directly and not reachable through any of the 13 shorthands section
//         6.2 defines -- it changes what the fact MEANS: they are no longer an isolated exception,
//         they are the PRECEDENT the general rule below generalizes from.
//       - 35 rows -- `ESC-1`'s own new rows, ALL zero-corpus BY DESIGN, not by anomaly, per
//         `docs/adr/0022-paridade-total-com-o-motor-substituido.md` and `docs/rmlx-subset.md` §7's
//         own rule ("se o motor que está sendo substituído aceita, o nosso aceita" -- if the
//         engine being replaced accepts it, ours accepts it, parity over the measured minimum).
//         Every one of the 35 is a real `RegisterProperty(...)` call in the PINNED build
//         (`glintfx/build/_deps/rmlui-src/Source/Core/StyleSheetSpecification.cpp:248-436`, not
//         the `examples/RmlUi` study clone), re-derived independently against that exact file by
//         this item's own delivery (not copied from a prior estimate) and cross-checked against
//         this repo's own `TODO.md` `ESC-1` entry, byte for byte, same 35 names, same order.
//       19 + 2 + 35 = 56; 51 + 56 = 107. ✓ No open discrepancy remains on either side of this
//       reconciliation -- the census side was already closed exact (64), and the registry side now
//       closes exact too (107), for the first time since this accounting was written.
// PT: UIX-PROP-REGISTRY -- implementação. Ver o próprio comentário de cabeçalho do
//     property_registry.hpp pro escopo/fronteira/racional-fail-high completos a que este arquivo
//     se prende.
//
//     A RECONCILIAÇÃO 107-VS-64-NOMES, atualizada pela `ESC-1` (2026-08-07, +35 linhas, 72 -> 107)
//     -- a reconciliação 72-vs-64 ORIGINAL que o próprio briefing deste item um dia pediu pra
//     fechar (a pergunta em aberto que a própria entrada `UIX-RCSS-SPEC` do TODO.md do
//     docs/uix-rcss.md deixou sem resolver: "a reconciliação exata... fica registrada como
//     discrepância aberta na seção 6, não resolvida byte a byte"). Contada à mão contra a própria
//     lista de 64 nomes da seção 3 do `/var/tmp/censo-rcss-qa1/censo.md` (também hardcoded, à
//     parte, no próprio `test_every_measured_property_is_covered` do
//     tests/uix_style/property_registry_sanity.cpp -- aquele teste é a versão VIVA, checada por
//     máquina, deste mesmo raciocínio; este comentário é a narrativa em prosa do resultado
//     idêntico). **O lado do censo é FIXO, a `ESC-1` não tocou nele** -- nenhuma das 35 linhas
//     novas colide com um nome medido pelo censo nem com um nome de shorthand medido, verificado
//     direto (a nova contagem fecha exata, nenhuma discrepância aberta nova):
//       - 51 dos 64 nomes do censo são entradas diretas na própria tabela de 107 linhas deste
//         arquivo (inalterado pela `ESC-1` -- nenhuma das 35 linhas novas compartilha nome com uma
//         entrada do censo).
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
//     A OUTRA direção -- as próprias 107 linhas desta tabela menos as 51 medidas-por-nome direto --
//     deixa 56 linhas sem medição direta de censo, agora fechando exato em TRÊS baldes (a versão
//     pré-`ESC-1` deste comentário só tinha dois, mais um resto inexplicado):
//       - 19 linhas existem SÓ porque um shorthand medido expande pra elas (inalterado pela
//         `ESC-1`): 12 linhas `border-*` (`border-{top,right,bottom,left}-{width,color}` da
//         própria expansão de `border`/`border-color`, `border-{top,right,bottom,left}-
//         {left,right}-radius` da própria expansão de `border-radius`) -- todas alcançáveis pelos
//         5 nomes de shorthand `border*` do censo acima; `row-gap`/`column-gap` (2) -- de `gap`;
//         `flex-basis`/`flex-grow`/`flex-shrink` (3) -- de `flex`; `padding-bottom`/`padding-left`
//         (2) -- de `padding` (os PRÓPRIOS irmãos `padding-top`/`padding-right` SÃO medidos
//         diretamente por nome no censo, em 17 e 8 instâncias respectivamente --
//         `padding-bottom`/`-left` simplesmente nunca calham de ser escritos sozinhos em lugar
//         nenhum do corpus, só sempre pelo shorthand `padding`). 12+2+3+2 = 19.
//       - `max-height`/`max-width` (2) -- RECLASSIFICADAS pela `ESC-1`, não re-explicadas por um
//         mecanismo NOVO: este par costumava ser "a única entrada de registro com zero
//         justificativa de corpus", uma anomalia isolada, não-explicada, que uma revisão anterior
//         deste arquivo reportava como pergunta genuinamente aberta (verificada duas vezes contra o
//         corpus real, ver o próprio histórico git deste comentário pra narrativa de verificação
//         original, preservada no log de commit em vez de repetida aqui). A `ESC-1` não muda esse
//         fato subjacente -- estas duas linhas continuam não medidas diretamente e não alcançáveis
//         por nenhum dos 13 shorthands da seção 6.2 -- muda o que o fato SIGNIFICA: deixam de ser
//         uma exceção isolada, viram o PRECEDENTE de que a regra geral abaixo generaliza.
//       - 35 linhas -- as próprias linhas novas da `ESC-1`, TODAS zero-corpus POR DESENHO, não por
//         anomalia, pela própria `docs/adr/0022-paridade-total-com-o-motor-substituido.md` e pela
//         própria regra da §7 do `docs/rmlx-subset.md` ("se o motor que está sendo substituído
//         aceita, o nosso aceita" -- paridade sobre o mínimo medido). Cada uma das 35 é uma
//         chamada `RegisterProperty(...)` real no build FIXADO
//         (`glintfx/build/_deps/rmlui-src/Source/Core/StyleSheetSpecification.cpp:248-436`, não o
//         clone de estudo `examples/RmlUi`), re-derivada de forma independente contra aquele
//         arquivo exato pela própria entrega deste item (não copiada de uma estimativa anterior) e
//         cruzada contra o próprio item `ESC-1` do `TODO.md` deste repo, byte a byte, os mesmos 35
//         nomes, a mesma ordem.
//       19 + 2 + 35 = 56; 51 + 56 = 107. ✓ Nenhuma discrepância aberta permanece de nenhum dos dois
//       lados desta reconciliação -- o lado do censo já fechava exato (64), e o lado do registro
//       agora fecha exato também (107), pela primeira vez desde que esta contagem foi escrita.
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
    one("-rmlui-direction", "auto", true, KW),
    one("-rmlui-language", "", true, STR),
    one("align-content", "stretch", false, KW),
    one("align-items", "stretch", false, KW),
    one("align-self", "auto", false, KW),
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
    two("caret-color", "auto", true, KW, COL),
    one("clear", "none", false, KW),
    two("clip", "auto", false, KW, NUM),
    one("color", "white", true, COL),
    one("column-gap", "0px", false, LEN),
    one("cursor", "", true, STR),
    one("decorator", "", false, CMP),
    one("display", "inline", false, KW),
    one("drag", "none", false, KW),
    one("fill-image", "", false, STR),
    one("filter", "", false, CMP),
    two("flex-basis", "auto", false, KW, LP),
    one("flex-direction", "row", false, KW),
    one("flex-grow", "0", false, NUM),
    one("flex-shrink", "1", false, NUM),
    one("flex-wrap", "nowrap", false, KW),
    one("float", "none", false, KW),
    // EN: ⚠️ `inherited: true`, surprising -- see property_registry.hpp's own header, "Fail-high
    //     policy" paragraph's sibling note; docs/uix-rcss.md section 6.1's own ⚠️ flag, confirmed
    //     at the RmlUi call site, not guessed.
    // PT: ⚠️ `inherited: true`, surpreendente -- ver o próprio parágrafo irmão de "Política
    //     fail-high" do cabeçalho do property_registry.hpp; a própria flag ⚠️ da seção 6.1 do
    //     docs/uix-rcss.md, confirmada no próprio call site do RmlUi, não chutada.
    one("focus", "auto", true, KW),
    one("font-effect", "", true, CMP),
    one("font-family", "", true, STR),
    one("font-kerning", "auto", true, KW),
    one("font-size", "12px", true, LEN),
    one("font-style", "normal", true, KW),
    two("font-weight", "normal", true, KW, NUM),
    two("height", "auto", false, KW, LP),
    one("image-color", "white", false, COL),
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
    two("nav-down", "none", false, KW, STR),
    two("nav-left", "none", false, KW, STR),
    two("nav-right", "none", false, KW, STR),
    two("nav-up", "none", false, KW, STR),
    // EN: ⚠️ `inherited: true`, NOT how real CSS `opacity` behaves -- see property_registry.hpp's
    //     own header, "Fail-high policy" paragraph's sibling note; docs/uix-rcss.md section 6.1's
    //     own ⚠️ flag.
    // PT: ⚠️ `inherited: true`, NÃO é como o `opacity` do CSS real se comporta -- ver o próprio
    //     parágrafo irmão de "Política fail-high" do cabeçalho do property_registry.hpp; a própria
    //     flag ⚠️ da seção 6.1 do docs/uix-rcss.md.
    one("opacity", "1", true, NUM),
    one("overflow-x", "visible", false, KW),
    one("overflow-y", "visible", false, KW),
    one("overscroll-behavior", "auto", false, KW),
    one("padding-bottom", "0px", false, LP),
    one("padding-left", "0px", false, LP),
    one("padding-right", "0px", false, LP),
    one("padding-top", "0px", false, LP),
    two("perspective", "none", false, KW, LEN),
    two("perspective-origin-x", "50%", false, KW, LP),
    two("perspective-origin-y", "50%", false, KW, LP),
    one("pointer-events", "auto", true, KW),
    one("position", "static", false, KW),
    two("right", "auto", false, KW, LP),
    one("ripple-origin-x", "0", false, NUM),
    one("ripple-origin-y", "0", false, NUM),
    one("ripple-phase", "0", false, NUM),
    one("ripple-strength", "0", false, NUM),
    one("ripple-width", "48", false, NUM),
    one("row-gap", "0px", false, LEN),
    one("scrollbar-margin", "0", false, LEN),
    one("tab-index", "none", false, KW),
    one("text-align", "left", true, KW),
    // EN: ⚠️ `inherited: true`, NOT how real CSS `text-decoration` behaves -- real CSS declares
    //     `text-decoration` `Inherited: no` (browsers continue decoration LINES across descendant
    //     text runs via a separate, rendering-level propagation rule, not the `inherited` cascade
    //     mechanism this flag models); RmlUi registers `inherited: true`, confirmed directly at
    //     the RmlUi call site (`StyleSheetSpecification.cpp:362`), not guessed -- see
    //     property_registry.hpp's own header, "Fail-high policy" paragraph's sibling note;
    //     docs/uix-rcss.md section 6.1's own ⚠️ flag (a THIRD entry, alongside focus/opacity above).
    // PT: ⚠️ `inherited: true`, NÃO é como o `text-decoration` do CSS real se comporta -- o CSS
    //     real declara `text-decoration` `Inherited: no` (navegadores continuam as LINHAS de
    //     decoração através de trechos de texto descendentes por uma regra de propagação separada,
    //     em nível de renderização, não o próprio mecanismo de cascata da flag `inherited` que
    //     este campo modela); o RmlUi registra `inherited: true`, confirmado direto no próprio call
    //     site do RmlUi (`StyleSheetSpecification.cpp:362`), não chutado -- ver o próprio parágrafo
    //     irmão de "Política fail-high" do cabeçalho do property_registry.hpp; a própria flag ⚠️ da
    //     seção 6.1 do docs/uix-rcss.md (uma TERCEIRA entrada, ao lado de focus/opacity acima).
    one("text-decoration", "none", true, KW),
    two("text-overflow", "clip", false, KW, STR),
    one("text-transform", "none", true, KW),
    two("top", "auto", false, KW, LP),
    one("transform", "none", false, CMP),
    two("transform-origin-x", "50%", false, KW, LP),
    two("transform-origin-y", "50%", false, KW, LP),
    one("transform-origin-z", "0", false, LEN),
    one("transition", "none", false, CMP),
    two("vertical-align", "baseline", false, KW, LP),
    one("visibility", "visible", false, KW),
    one("white-space", "normal", true, KW),
    two("width", "auto", false, KW, LP),
    one("word-break", "normal", true, KW),
    two("z-index", "auto", false, KW, NUM),
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
