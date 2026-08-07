// SPDX-License-Identifier: Apache-2.0
// EN: UIX-RCSS-DUMP-B -- side B of the `RMLX-2` style differential oracle: walks glintfx's OWN
//     RCSS engine (`lexer.hpp` -> `parser.hpp` -> `selector_match.hpp` -> `cascade.hpp` ->
//     `value_compute.hpp`, all five bricks already delivered by earlier `RMLX-2` slices) and
//     serializes the result into the canonical, byte-exact dump format `docs/uix-rcss.md`
//     ("the spec" hereafter) defines. Side A (`glintfx/src/rml/rcss_dump.{hpp,cpp}`) walks the
//     REAL RmlUi `Style::ComputedValues` tree instead, confined to `glintfx/src/rml/`, written by
//     a DIFFERENT agent who never read this file (nor did this file's own author read side A --
//     see the spec's own "Why this document exists" section for why that separation is load-
//     bearing, not a formality). A future `S7`-equivalent oracle slice diffs the two dumpers'
//     output over the shared corpus; a divergence there is the spec's own three-class ledger
//     (section 14) to classify, never something this file's own author decides unilaterally.
//
//     PUBLIC SURFACE, TWO FUNCTIONS, MIRRORING THE `RMLX-1`/S6b PRECEDENT
//     (`glintfx/src/uix/dom/dumper.hpp`): `canonical_print` prints ONE already-cascaded property
//     (spec sections 6-9); `dump_style` walks a whole tree under the spec's own 2-member `STATE`
//     matrix (section 4) and prints the whole file. Neither takes RML/RCSS SOURCE TEXT -- both
//     take an already-parsed `StyleSheet` (`parser.hpp`'s own `SheetParseResult::sheet`) and an
//     already-built `glintfx::uix::Element` tree (`glintfx::uix::parse_document`'s own
//     `Document::body()`) -- same "the dumper is not a parser" boundary the DOM sibling's own
//     header states for itself; the caller (a future oracle harness, or this item's own test
//     suite) owns calling `parse_document`/`parse_stylesheet` first.
//
//     THE STAGE THIS FILE OWNS, NAMED EXPLICITLY BY THE BRIEF THAT DISPATCHED IT: the pipeline is
//     "lexer -> parser -> casador -> cascata -> valor computado -> dump", six named stages
//     (`cascade.hpp`'s own header comment, "Deliberately NOT this file's job"). `cascade.hpp`
//     stops BEFORE "valor computado" on purpose -- `ComputedProperty::value` is always the raw,
//     pre-canonical winning declaration text (or the registry's own raw `initial_value`), never
//     passed through `value_compute.hpp`. THIS file is "valor computado" AND "dump" both: it
//     dispatches each of the 72 registered properties' own raw winning text through the correct
//     `value_compute.hpp` function for its `ValueDomain` (spec section 7), including resolving
//     which of TWO domains a `has_alternate_domain` property's raw text actually is (spec's own
//     table cells read "keyword(auto) or length-percent" etc. -- `property_registry.hpp` states
//     outright it does NOT decide this, "needs the actual value text, parser territory" -- THIS
//     file is that territory; see dumper.cpp's own header for the exact, narrow classifier and why
//     it is scoped the way it is), then serializes the whole tree under both `STATE` blocks.
//
//     WHAT THIS FILE DOES NOT INVENT, REPORTED RATHER THAN GUESSED (this task's own brief: "se a
//     spec não bastar... pare e reporte, não improvise"): `value_compute.hpp` deliberately does
//     NOT implement `animation`'s own composite grammar (section 9.3) -- its own header names a
//     real spec-vs-upstream gap (the spec's grammar never names a `delay` field upstream's own
//     `Animation` struct actually has) and refuses to guess a byte-exact form. This file inherits
//     that boundary rather than inventing a grammar the delivered `value_compute.hpp` itself
//     declined to build: an `animation` declaration is dumped as `"none"` (the property's own
//     section 6.1 initial value) regardless of what was authored, via the SAME fail-high path
//     every other genuinely-unparseable value takes (see dumper.cpp's own header, "The animation
//     gap"). This is a REPORTED, DELIBERATE limitation, not a silent one -- restated in this
//     item's own delivery notes and `TODO.md` entry, not hidden here.
//
//     KEYWORD-DOMAIN VALUES ARE NOT VALIDATED AGAINST AN ENUMERATED LEGAL SET, BY THE SAME
//     UPSTREAM BOUNDARY: `value_compute.hpp`'s own header states "keyword is the caller's job --
//     it is already the exact registered string, nothing to compute" and provides NO
//     keyword-validating function at all (unlike `parse_color`/`parse_length`/`parse_percent`,
//     which all signal `Invalid`). This file therefore prints a `Keyword`-domain raw value
//     VERBATIM, trusting it is well-formed -- a hostile or malformed keyword (a typo'd
//     `display: blocc;`, say) is NOT caught here and does not fall back to the registry's own
//     initial value the way an unparseable length/color/percent does. This is a genuine,
//     documented divergence from strict upstream fail-high behaviour (spec section 11) that this
//     item's own delivered dependency (`value_compute.hpp`) structurally does not support without
//     this file inventing an enumerated-value validator the spec's own section 6.1 table does not
//     fully spell out per property (some cells DO enumerate, e.g. "keyword(none,auto)"; most just
//     say "keyword"). Reported here, in dumper.cpp's own header, and in this item's own delivery
//     notes -- not silently patched over by guessing a validation table the spec never gave this
//     item license to invent.
//
//     TETO, EXPLICIT, MIRRORING `value_compute.hpp`'s OWN "never silently truncate" discipline:
//     this file introduces no NEW recursion depth of its own -- `dump_style`'s own tree walk
//     recurses exactly as deep as `cascade_tree` already does (`dom_tree.hpp`'s own
//     `kMaxElementDepth`, 256, already the ceiling both this file's own path-collection walk and
//     `cascade_tree`'s own walk share), and every per-property computation is O(1) work over an
//     already-bounded raw-value length (`value_compute.hpp`'s own `kMaxRawValueBytes`, 8 KiB).
//
//     CONFINEMENT (this item's own #1 rule, restated so a reader who skips the brief still sees
//     it): zero `#include <RmlUi/...>` anywhere in this file or its `.cpp` -- `glintfx/src/rml/` is
//     the only directory in this repo with permission for that, and this file is not in it.
//     `tools/check_rml_whitelist.sh` is the machine-checked guard.
//
//     NAMESPACE: `glintfx::uix::style`, same module as every sibling header in this directory.
// PT: UIX-RCSS-DUMP-B -- lado B do oráculo diferencial de estilo da `RMLX-2`: percorre o motor RCSS
//     PRÓPRIO da glintfx (`lexer.hpp` -> `parser.hpp` -> `selector_match.hpp` -> `cascade.hpp` ->
//     `value_compute.hpp`, os cinco tijolos já entregues por fatias anteriores da `RMLX-2`) e
//     serializa o resultado no formato de dump canônico, byte-exato, que o `docs/uix-rcss.md` ("a
//     spec" daqui em diante) define. O lado A (`glintfx/src/rml/rcss_dump.{hpp,cpp}`) percorre a
//     árvore `Style::ComputedValues` REAL do RmlUi em vez disso, confinado a `glintfx/src/rml/`,
//     escrito por um agente DIFERENTE que nunca leu este arquivo (nem o próprio autor deste arquivo
//     leu o lado A -- ver a própria seção "Por que este documento existe" da spec pro porquê dessa
//     separação ser estrutural, não formalidade). Uma futura fatia de oráculo equivalente à `S7`
//     faz o diff da saída dos dois dumpers sobre o corpus compartilhado; uma divergência ali é o
//     próprio registro de três classes da spec (seção 14) pra classificar, nunca algo que o próprio
//     autor deste arquivo decide unilateralmente.
//
//     SUPERFÍCIE PÚBLICA, DUAS FUNÇÕES, ESPELHANDO O PRECEDENTE DA `RMLX-1`/S6b
//     (`glintfx/src/uix/dom/dumper.hpp`): `canonical_print` imprime UMA propriedade já cascateada
//     (seções 6-9 da spec); `dump_style` percorre uma árvore inteira sob a própria matriz `STATE`
//     de 2 membros da spec (seção 4) e imprime o arquivo inteiro. Nenhuma das duas recebe TEXTO
//     FONTE RML/RCSS -- as duas recebem um `StyleSheet` já parseado (o próprio
//     `SheetParseResult::sheet` do parser.hpp) e uma árvore `glintfx::uix::Element` já construída
//     (o próprio `Document::body()` do `glintfx::uix::parse_document`) -- mesma fronteira "o dumper
//     não é um parser" que o próprio cabeçalho do irmão DOM declara pra si mesmo; o chamador (um
//     futuro harness de oráculo, ou a própria suíte de teste deste item) é dono de chamar
//     `parse_document`/`parse_stylesheet` primeiro.
//
//     O ESTÁGIO QUE ESTE ARQUIVO POSSUI, NOMEADO EXPLICITAMENTE PELO BRIEFING QUE O DESPACHOU: o
//     pipeline é "lexer -> parser -> casador -> cascata -> valor computado -> dump", seis estágios
//     nomeados (o próprio comentário de cabeçalho do cascade.hpp, "Deliberadamente NÃO é trabalho
//     deste arquivo"). O `cascade.hpp` para ANTES de "valor computado" de propósito --
//     `ComputedProperty::value` é sempre o próprio texto cru, pré-canônico, da declaração
//     vencedora (ou o próprio `initial_value` cru do registro), nunca passado pelo
//     `value_compute.hpp`. ESTE arquivo é "valor computado" E "dump" ao mesmo tempo: despacha o
//     próprio texto cru vencedor de cada uma das 72 propriedades registradas pela função certa do
//     `value_compute.hpp` pro próprio `ValueDomain` dela (seção 7 da spec), inclusive resolvendo a
//     qual dos DOIS domínios o texto cru de uma propriedade `has_alternate_domain` de fato pertence
//     (as próprias células da tabela da spec dizem "keyword(auto) ou length-percent" etc. -- o
//     `property_registry.hpp` declara categoricamente que NÃO decide isto, "precisa do próprio
//     texto de valor, território de parser" -- ESTE arquivo é esse território; ver o próprio
//     cabeçalho do dumper.cpp pro classificador exato, estreito, e por que é escopado do jeito que
//     é), depois serializa a árvore inteira sob os dois blocos `STATE`.
//
//     O QUE ESTE ARQUIVO NÃO INVENTA, REPORTADO EM VEZ DE CHUTADO (o próprio briefing desta
//     tarefa: "se a spec não bastar... pare e reporte, não improvise"): o `value_compute.hpp`
//     deliberadamente NÃO implementa a própria gramática composta de `animation` (seção 9.3) -- o
//     próprio cabeçalho dele nomeia uma lacuna real spec-vs-upstream (a gramática da spec nunca
//     nomeia um campo `delay` que o próprio struct `Animation` do upstream de fato tem) e se
//     recusa a chutar uma forma byte-exata. Este arquivo herda essa fronteira em vez de inventar
//     uma gramática que o próprio `value_compute.hpp` entregue recusou construir: uma declaração
//     `animation` é dumpada como `"none"` (o próprio valor inicial da seção 6.1 daquela
//     propriedade) independente do que foi autorado, pelo MESMO caminho fail-high que todo outro
//     valor genuinamente-não-parseável toma (ver o próprio cabeçalho do dumper.cpp, "A lacuna do
//     animation"). Esta é uma limitação REPORTADA, DELIBERADA, não silenciosa -- restatada nas
//     próprias notas de entrega deste item e no próprio item do `TODO.md`, não escondida aqui.
//
//     VALORES DE DOMÍNIO-KEYWORD NÃO SÃO VALIDADOS CONTRA UM CONJUNTO LEGAL ENUMERADO, PELA MESMA
//     FRONTEIRA DO UPSTREAM: o próprio cabeçalho do `value_compute.hpp` declara "keyword é
//     trabalho do chamador -- já é a própria string registrada exata, nada a computar" e não
//     fornece função nenhuma de validação de keyword (diferente de
//     `parse_color`/`parse_length`/`parse_percent`, que os três sinalizam `Invalid`). Este arquivo
//     portanto imprime um valor cru de domínio `Keyword` VERBATIM, confiando que está bem-formado
//     -- um keyword hostil ou malformado (um `display: blocc;` digitado errado, digamos) NÃO é
//     pego aqui e não cai pro próprio valor inicial do registro do jeito que um
//     comprimento/cor/porcentagem não-parseável cai. Esta é uma divergência genuína, documentada,
//     do comportamento fail-high estrito do upstream (seção 11 da spec) que a própria dependência
//     entregue deste item (`value_compute.hpp`) estruturalmente não suporta sem este arquivo
//     inventar um validador de valor-enumerado que a própria tabela da seção 6.1 da spec não
//     soletra por completo por propriedade (algumas células ENUMERAM, ex. "keyword(none,auto)"; a
//     maioria só diz "keyword"). Reportado aqui, no próprio cabeçalho do dumper.cpp, e nas próprias
//     notas de entrega deste item -- não remendado em silêncio chutando uma tabela de validação que
//     a spec nunca deu licença a este item pra inventar.
//
//     TETO, EXPLÍCITO, ESPELHANDO A PRÓPRIA DISCIPLINA "nunca trunca em silêncio" do
//     `value_compute.hpp`: este arquivo não introduz recursão NOVA própria nenhuma -- a própria
//     travessia de árvore do `dump_style` recursa exatamente tão fundo quanto o `cascade_tree` já
//     recursa (o próprio `kMaxElementDepth` do dom_tree.hpp, 256, já o teto que TANTO a própria
//     travessia de coleta-de-caminho deste arquivo QUANTO a própria travessia do `cascade_tree`
//     compartilham), e todo cálculo por-propriedade é trabalho O(1) sobre um comprimento de
//     valor-cru já limitado (o próprio `kMaxRawValueBytes` do value_compute.hpp, 8 KiB).
//
//     CONFINAMENTO (a própria regra nº 1 deste item, restatada pra um leitor que pula o briefing
//     ainda assim ver): zero `#include <RmlUi/...>` em lugar nenhum deste arquivo ou do próprio
//     .cpp -- `glintfx/src/rml/` é o único diretório deste repo com permissão pra isso, e este
//     arquivo não está nele. `tools/check_rml_whitelist.sh` é a guarda verificada-por-máquina.
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo de todo header irmão deste diretório.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <string>

#include "uix/dom/dom_tree.hpp"
#include "uix/style/cascade.hpp"
#include "uix/style/parser.hpp"

namespace glintfx::uix::style {

// EN: Prints `prop`'s own canonical `<canonical-value>` half (spec sections 6-9) -- NOT the
//     `<property-name>=` prefix, which `dump_style` below prints itself from `prop.name` directly
//     (property names are structural identifiers, never escaped or computed, per spec section 7's
//     own closing paragraph). Looks `prop.name` up in `property_registry.hpp`'s own closed table
//     (always found -- `prop` always comes from a `ComputedStyle` `cascade_tree`/
//     `compute_element_style` produced, which is itself built by walking `all_properties()`),
//     resolves which domain a `has_alternate_domain` property's raw text actually is (dumper.cpp's
//     own header names the exact classifier), dispatches to the matching `value_compute.hpp`
//     function, and on `ValueComputeStatus::Invalid` retries ONCE against the property's own
//     registry `initial_value` (spec section 11's own "whole declaration reverts" fail-high rule,
//     applied at print time -- see this file's header comment, "keyword-domain values are not
//     validated", for the ONE domain this retry structurally cannot help). `dp_ratio` is only
//     consulted by domains/composite-grammars that resolve an absolute length (spec section 8.1).
//     Exposed standalone (not `dump_style`-local) for the same "a future caller might want just
//     this" reasoning the DOM sibling's own `escape()` is exposed for.
// PT: Imprime a própria metade `<valor-canônico>` de `prop` (seções 6-9 da spec) -- NÃO o prefixo
//     `<nome-propriedade>=`, que o `dump_style` abaixo imprime ele mesmo a partir de `prop.name`
//     direto (nomes de propriedade são identificadores estruturais, nunca escapados nem
//     computados, pelo próprio parágrafo de fechamento da seção 7 da spec). Busca `prop.name` na
//     própria tabela fechada do property_registry.hpp (sempre achado -- `prop` sempre vem de um
//     `ComputedStyle` que o `cascade_tree`/`compute_element_style` produziu, que por sua vez é
//     construído percorrendo `all_properties()`), resolve a qual domínio o texto cru de uma
//     propriedade `has_alternate_domain` de fato pertence (o próprio cabeçalho do dumper.cpp nomeia
//     o classificador exato), despacha pra função certa do value_compute.hpp, e em
//     `ValueComputeStatus::Invalid` retenta UMA VEZ contra o próprio `initial_value` de registro da
//     propriedade (a própria regra fail-high "a declaração inteira reverte" da seção 11 da spec,
//     aplicada em tempo de impressão -- ver o comentário de cabeçalho deste arquivo, "valores de
//     domínio-keyword não são validados", pro ÚNICO domínio em que este retry estruturalmente não
//     ajuda). `dp_ratio` só é consultado por domínios/gramáticas-compostas que resolvem um
//     comprimento absoluto (seção 8.1 da spec). Exposta standalone (não local ao `dump_style`) pelo
//     mesmo raciocínio "um futuro chamador pode querer só isto" que expõe o `escape()` do irmão DOM.
std::string canonical_print(const ComputedProperty& prop, float dp_ratio);

// EN: Serializes `sheet` cascaded over `root`'s own subtree into the spec's own canonical dump
//     format: two `STATE` blocks (`none` then `hover-all`, spec section 4's own fixed order, NOT
//     the byte-wise sort rule section 6 uses elsewhere -- `dumper.cpp`'s own header names why this
//     is hardcoded, not derived), each a full pre-order-depth-first `PROPS`/`PROP` enumeration
//     (spec section 3) -- one line per fact, every line `\n`-terminated including the last (spec
//     section 3's own "the dump file always ends with a single trailing newline" clause), no blank
//     line anywhere. Path addressing is `docs/uix-dom.md` section 3's own scheme, reused unchanged
//     per spec section 2 ("this dump walks the SAME tree, addressed the SAME way"): `body` is the
//     literal root, every descendant is `body` + one `/<n>` per level, `n` the 0-based position
//     among ALL of the parent's own surviving children (text nodes included in the COUNT, per that
//     same section -- they simply never get their own `PROP` block, matching spec section 2's own
//     "only element nodes carry PROP records" clause). Never throws, never fails -- there is no
//     `Element`/`StyleSheet` pair this function cannot dump (a malformed individual declaration's
//     own raw text falls back to its registry initial value per `canonical_print`'s own fail-high
//     retry, never a hard stop for the whole dump).
// PT: Serializa `sheet` cascateado sobre a própria subárvore de `root` no formato de dump canônico
//     da spec: dois blocos `STATE` (`none` depois `hover-all`, a própria ordem fixa da seção 4 da
//     spec, NÃO a regra de sort byte-a-byte que a seção 6 usa em outro lugar -- o próprio
//     cabeçalho do dumper.cpp nomeia por que isto é hardcoded, não derivado), cada um uma
//     enumeração `PROPS`/`PROP` completa, pré-ordem, profundidade-primeiro (seção 3 da spec) -- um
//     fato por linha, toda linha terminada em `\n` inclusive a última (a própria cláusula "o
//     arquivo de dump sempre termina com um único newline final" da seção 3 da spec), nenhuma
//     linha em branco em lugar nenhum. Endereçamento de caminho é o próprio esquema da seção 3 do
//     docs/uix-dom.md, reusado sem mudança pela própria seção 2 da spec ("este dump percorre a
//     MESMA árvore, endereçada da MESMA forma"): `body` é a raiz literal, todo descendente é
//     `body` + um `/<n>` por nível, `n` a posição 0-based entre TODOS os próprios filhos
//     sobreviventes do pai (nós de texto incluídos na CONTAGEM, pela mesma seção -- eles
//     simplesmente nunca ganham bloco `PROP` próprio, batendo com a própria cláusula "só nós
//     elemento carregam registros PROP" da seção 2 da spec). Nunca lança exceção, nunca falha --
//     não existe par `Element`/`StyleSheet` que esta função não consiga dumpar (o próprio texto
//     cru de uma declaração individual malformada cai pro próprio valor inicial de registro dela
//     via o próprio retry fail-high do `canonical_print`, nunca uma parada dura pro dump inteiro).
//
// EN: `UIX-EM-UNIT`, added here rather than to `canonical_print()`'s own signature above: this
//     function's own `font-size` line, ONLY, is printed from a resolved-pixel value this function
//     computes itself by walking `root`'s own subtree top-down (parent before child, the same order
//     `cascade_tree()` already produces) and feeding each node's own resolved font-size in as the
//     NEXT node's `value_compute.hpp::parse_font_size()` own `parent_font_size_px` argument -- the
//     one piece of ancestor context `em` needs that `canonical_print()`'s own per-property,
//     ancestor-blind signature structurally cannot supply (see this file's own `canonical_print()`
//     doc-comment above, unchanged). Every OTHER property still goes through `canonical_print()`
//     exactly as before -- this is a single, named exception, not a rewrite of the print loop's own
//     general shape. `UIX-ORACLE-MEDICAO`'s own residuo B is the fixture this closes:
//     `fonteng_sup_scene.rml`'s own `.sup{font-size:0.7em}` over `body{font-size:64px}` now dumps
//     `44.8000px`, matching side A (real RmlUi), not the `12.0000px` registry-initial fallback this
//     function's own `font-size` line used to fall back to for any `em`-shaped value.
// PT: `UIX-EM-UNIT`, acrescentado aqui em vez da própria assinatura do `canonical_print()` acima: a
//     própria linha de `font-size` desta função, SÓ ela, é impressa a partir de um valor de pixel já
//     resolvido que esta própria função computa percorrendo a própria subárvore de `root` de cima pra
//     baixo (pai antes de filho, a mesma ordem que o `cascade_tree()` já produz) e alimentando o
//     próprio font-size resolvido de cada nó como o próprio argumento `parent_font_size_px` do
//     `value_compute.hpp::parse_font_size()` do PRÓXIMO nó -- a única peça de contexto ancestral que
//     o `em` precisa e que a própria assinatura por-propriedade, cega-a-ancestral, do
//     `canonical_print()` estruturalmente não consegue fornecer (ver o próprio doc-comment do
//     `canonical_print()` acima, inalterado). Toda OUTRA propriedade ainda passa pelo
//     `canonical_print()` exatamente como antes -- esta é uma única exceção nomeada, não uma
//     reescrita da própria forma geral do laço de impressão. O próprio resíduo B da
//     `UIX-ORACLE-MEDICAO` é a fixture que isto fecha: o próprio `.sup{font-size:0.7em}` sobre
//     `body{font-size:64px}` do `fonteng_sup_scene.rml` agora dumpa `44.8000px`, casando com o lado A
//     (RmlUi real), não o fallback `12.0000px` de inicial-de-registro pro qual a própria linha de
//     `font-size` desta função caía pra qualquer valor com forma `em`.
std::string dump_style(const StyleSheet& sheet, const glintfx::uix::Element& root, float dp_ratio);

} // namespace glintfx::uix::style
