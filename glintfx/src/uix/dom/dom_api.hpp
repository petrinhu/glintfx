// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S4 -- the public, ergonomic, BY-ID DOM read/write API this arc's own dom_tree.hpp
//     (S2, dfb9fd9) explicitly deferred to this slice ("the public, ergonomic DOM read/write API
//     glintfx's facade (`App`/`UiLayer`) will eventually expose (S4)"). This is the layer S7's
//     future RmlUi-vs-uix parity work will actually call: `glintfx::uix::set_text/add_class/
//     remove_class`, each taking a `Document&` and an `id`, mirroring the EXACT signatures
//     `App`/`UiLayer::set_text/add_class/remove_class` already expose today over the RmlUi-backed
//     engine (`glintfx/include/glintfx/ui_layer.hpp` lines ~885-946, `glintfx/src/rml/
//     bootstrap.cpp` lines ~1771-1819) -- this module's own job is to make the SAME three calls
//     work over `glintfx::uix::Document` instead, not to invent a different API shape.
//
//     SCOPE, EXACTLY AS THE BRIEF DELIMITS IT (four operations named, three are write, one is the
//     id lookup underneath all three):
//       - `set_text(doc, id, text)`     -- find by id, write text content.
//       - `add_class(doc, id, cls)`     -- find by id, delegate to `Element::add_class`.
//       - `remove_class(doc, id, cls)`  -- find by id, delegate to `Element::remove_class`.
//       - the id lookup itself, `Document::body().find_by_id(id)` (dom_tree.hpp, S2) -- this
//         module adds NO lookup of its own; it is a thin, id-resolving wrapper over what S2
//         already built.
//
//     DELIBERATELY NOT THIS MODULE'S JOB (see this slice's own brief for the reasoning behind
//     each boundary -- restated here only as WHAT, not WHY, same "why lives once, in the brief/
//     ADR, not duplicated per function" discipline dom_tree.cpp already follows for dom_tree.hpp):
//       - `set_property(id, prop, value)` -- needs the RCSS cascade (`RMLX-2`). This module has no
//         notion of a stylesheet, selector, or computed value.
//       - `get_element_box`, `scroll_element_into_view` (both overloads), `get/set_element_scroll_
//         top`, `get_element_scroll_height`, `get_element_client_height` -- all return PIXELS,
//         which do not exist before layout (`RMLX-4`/`RMLX-5`). This module has no notion of a box
//         model, a viewport, or a scroll offset.
//       - The nine `set_*_callback` methods and `set_focus(id)` -- need hit-testing and/or a hover
//         chain, an event-dispatch concern of its own future wave. This module never walks the
//         tree looking for "what's under the pointer" or "what's currently focused" -- it only
//         ever resolves an id the CALLER already knows, exactly like `Element::find_by_id` itself.
//
//     THREE DECISIONS THIS SLICE MAKES, EACH DOCUMENTED AT ITS OWN FUNCTION BELOW IN FULL (this
//     paragraph is only the index):
//       (1) Duplicate-id policy is INHERITED, not re-decided -- `find_by_id`'s own "first
//           pre-order match wins" (dom_tree.hpp's "Lookup policy" paragraph) applies unchanged to
//           all three operations here, since all three resolve `id` through that exact function.
//           See `set_text`'s own doc-comment for the fixture-backed test that pins this.
//       (2) 🟢 `set_text` on an element that already has children -- ONCE this slice's real
//           boundary, CLOSED by UIX-REMOVE-CHILD (RMLX-1, 2026-08-05): `dom_tree.hpp`'s `Element`
//           now has `remove_child`/`clear_children`, so `set_text` clears ALL existing children
//           unconditionally before writing, exactly matching `Rml::Element::SetInnerRML`. See
//           `set_text`'s own doc-comment for the full before/after and the ONE deliberate new
//           behaviour change this closure introduces (object identity of an overwritten sole Text
//           child is no longer preserved -- see that doc-comment's own note on this).
//       (3) `add_class`/`remove_class` return `true` whenever `id` resolves AND `cls` is a
//           structurally valid single token -- REGARDLESS of whether the class set actually
//           changed (already-present add, already-absent remove are both idempotent NO-OPs that
//           still report `true`). This is not this module's own invention: it is the EXISTING
//           facade contract (`ui_layer.hpp` lines 936-945: "Removing a class the element never had
//           is a safe no-op inside RmlUi itself ... so this still returns `true` in that case"),
//           deliberately DIFFERENT from `Element::add_class`/`remove_class`'s own boolean
//           (`dom_tree.hpp`: "false if `cls` was already present -- caller-observable dedup
//           signal"). See `add_class`'s own doc-comment for the full reconciliation.
// PT: RMLX-1/S4 -- a API pública, ergonômica, POR-ID de leitura/escrita de DOM que o próprio
//     dom_tree.hpp (S2, dfb9fd9) deste arco deferiu explicitamente pra esta fatia ("a API pública,
//     ergonômica de leitura/escrita de DOM que a fachada da glintfx (`App`/`UiLayer`)
//     eventualmente vai expor (S4)"). Esta é a camada que o futuro trabalho de paridade RmlUi-vs-
//     uix da S7 vai realmente chamar: `glintfx::uix::set_text/add_class/remove_class`, cada uma
//     recebendo um `Document&` e um `id`, espelhando as MESMAS assinaturas que
//     `App`/`UiLayer::set_text/add_class/remove_class` já expõem hoje sobre o engine com base em
//     RmlUi (`glintfx/include/glintfx/ui_layer.hpp` linhas ~885-946, `glintfx/src/rml/
//     bootstrap.cpp` linhas ~1771-1819) -- o trabalho deste módulo é fazer as MESMAS três chamadas
//     funcionarem sobre `glintfx::uix::Document` em vez de inventar uma forma de API diferente.
//
//     ESCOPO, EXATAMENTE COMO O BRIEFING DELIMITA (quatro operações nomeadas, três são escrita,
//     uma é o lookup por id embaixo das três):
//       - `set_text(doc, id, text)`     -- acha por id, escreve o conteúdo de texto.
//       - `add_class(doc, id, cls)`     -- acha por id, delega ao `Element::add_class`.
//       - `remove_class(doc, id, cls)`  -- acha por id, delega ao `Element::remove_class`.
//       - o próprio lookup por id, `Document::body().find_by_id(id)` (dom_tree.hpp, S2) -- este
//         módulo não soma lookup nenhum próprio; é um wrapper fino, resolvedor-de-id, sobre o que
//         a S2 já construiu.
//
//     DELIBERADAMENTE NÃO É TRABALHO DESTE MÓDULO (ver o próprio briefing desta fatia pro
//     raciocínio atrás de cada fronteira -- restatado aqui só como O QUÊ, não POR QUÊ, mesma
//     disciplina "o porquê mora uma vez, no briefing/ADR, não duplicado por função" que o
//     dom_tree.cpp já segue pro dom_tree.hpp):
//       - `set_property(id, prop, value)` -- precisa da cascata RCSS (`RMLX-2`). Este módulo não
//         tem noção nenhuma de stylesheet, seletor, ou valor computado.
//       - `get_element_box`, `scroll_element_into_view` (as duas sobrecargas), `get/set_element_
//         scroll_top`, `get_element_scroll_height`, `get_element_client_height` -- todos devolvem
//         PIXELS, que não existem antes do layout (`RMLX-4`/`RMLX-5`). Este módulo não tem noção
//         nenhuma de box model, viewport, ou offset de scroll.
//       - Os nove métodos `set_*_callback` e `set_focus(id)` -- precisam de hit-test e/ou uma
//         cadeia de hover, uma preocupação de despacho-de-evento de sua própria onda futura. Este
//         módulo nunca percorre a árvore procurando "o que está sob o ponteiro" ou "o que está
//         focado agora" -- só resolve um id que QUEM CHAMA já conhece, exatamente como o próprio
//         `Element::find_by_id`.
//
//     TRÊS DECISÕES QUE ESTA FATIA TOMA, CADA UMA DOCUMENTADA NA PRÓPRIA FUNÇÃO ABAIXO POR
//     COMPLETO (este parágrafo é só o índice):
//       (1) A política de id duplicado é HERDADA, não re-decidida -- o próprio "primeiro
//           casamento em pré-ordem vence" do `find_by_id` (parágrafo "Lookup policy" do
//           dom_tree.hpp) vale sem mudança pras três operações aqui, já que as três resolvem `id`
//           através dessa mesma função. Ver o próprio doc-comment do `set_text` pro teste guiado
//           por fixture que fixa isto.
//       (2) 🟢 `set_text` num elemento que já tem filhos -- UMA VEZ a fronteira real desta fatia,
//           FECHADA pela UIX-REMOVE-CHILD (RMLX-1, 2026-08-05): o `Element` do `dom_tree.hpp`
//           agora tem `remove_child`/`clear_children`, então `set_text` limpa TODOS os filhos
//           existentes incondicionalmente antes de escrever, batendo exatamente com o
//           `Rml::Element::SetInnerRML`. Ver o próprio doc-comment do `set_text` pro antes/depois
//           completo e a ÚNICA mudança de comportamento deliberada nova que este fechamento
//           introduz (a identidade de objeto de um filho Text único sobrescrito não é mais
//           preservada -- ver a própria nota do doc-comment sobre isto).
//       (3) `add_class`/`remove_class` retornam `true` sempre que `id` resolve E `cls` é um token
//           único estruturalmente válido -- INDEPENDENTE de o conjunto de classe realmente ter
//           mudado (add já-presente, remove já-ausente são os dois NO-OPs idempotentes que ainda
//           reportam `true`). Isto não é invenção deste módulo: é o CONTRATO EXISTENTE da fachada
//           (`ui_layer.hpp` linhas 936-945: "Remover uma classe que o elemento nunca teve é um
//           no-op seguro dentro do próprio RmlUi ... então ainda retorna `true` nesse caso"),
//           deliberadamente DIFERENTE do próprio booleano de `Element::add_class`/`remove_class`
//           (`dom_tree.hpp`: "false se `cls` já estava presente -- sinal de dedup observável pelo
//           chamador"). Ver o próprio doc-comment do `add_class` pra reconciliação completa.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "uix/dom/dom_tree.hpp"

#include <string>
#include <string_view>

namespace glintfx::uix {

// EN: Replace an element's text content by id -- REPLICATES `Rml::Element::SetInnerRML` exactly
//     (`examples/RmlUi/Source/Core/Element.cpp:1166-1176`): unconditionally
//     `Element::clear_children()` (dom_tree.hpp, UIX-REMOVE-CHILD) FIRST, regardless of what was
//     there or whether the new `text` is empty, THEN `Element::append_child` a single new `Text`
//     node -- whose own existence filter already produces the correct "zero children" outcome for
//     an empty/whitespace-only `text`, matching `Factory::InstanceElementText`'s own
//     `only_white_space` filter exactly (`examples/RmlUi/Source/Core/Factory.cpp:330-341`). `text`
//     is stored byte-verbatim (no escaping, no entity handling -- this layer has no notion of
//     markup injection risk; that hardening, per `ui_layer.hpp` lines 886-895, is the RmlUi-backed
//     facade's own job when it eventually wraps this call, not this DOM-only layer's).
//
//     🟢 THIS REPLACES THE ORIGINAL S4 IMPLEMENTATION (`f15f1f8`), WHICH SPECIAL-CASED THREE
//     SHAPES (zero children / exactly one Text child / anything else) BECAUSE `dom_tree.hpp` HAD
//     NO REMOVAL PRIMITIVE. UIX-REMOVE-CHILD (RMLX-1, 2026-08-05) closed that gap by adding
//     `Element::remove_child`/`Element::clear_children`; this function now has exactly ONE code
//     path, for every shape the old three-way split had to distinguish:
//       - zero children before: `clear_children()` removes nothing (`0`), behaviourally identical
//         to the old shape (a).
//       - a sole `Text` child (the "overwrite" case): `clear_children()` DESTROYS that `Text`
//         object; the append that follows constructs a genuinely NEW one. 🔴 **This is a
//         deliberate, documented BEHAVIOUR CHANGE from the old contract**: the old shape (b)
//         guaranteed "SAME `Node`/`Text` object identity... preserved across the call" (mutation
//         in place via `Text::set_content`); that guarantee is GONE. This matches upstream exactly
//         (`RemoveChild` destroys the old node, `Factory::InstanceElementText` constructs a new
//         one) and no caller in this codebase today holds a `Text*`/`Node*` observer pointer
//         across a `set_text` call (`grep -rn set_text glintfx/tests` -- every call site either
//         discards the return or re-resolves by id afterward), so this is a safe closure, not a
//         silent regression -- but it IS new, and is called out here rather than left implicit.
//       - 2+ children of any kind, or a sole non-`Text` child (the old shape (c), 🔴 "the
//         load-bearing gap"): previously REFUSED (`false`, tree untouched). Now SUCCEEDS, exactly
//         like upstream -- every existing child is destroyed, the new `Text` (if any survives the
//         whitespace filter) is the ONLY child afterward. `test_set_text_element_with_non_text_or_
//         multiple_children_refused` (the test that PINNED the refusal) is REPLACED by
//         `test_set_text_element_with_non_text_or_multiple_children_replaces_all` -- see that
//         test's own comment in `dom_api_sanity.cpp` for why the old name is gone, not just its
//         body.
//       - `set_text(doc, id, "")` on an element that already holds a sole `Text` child: previously
//         left an EMPTY `Text` RESIDUAL (`child_count()` stayed `1`) -- the ONE divergence from
//         upstream this slice's own prior doc-comment named explicitly, pinned by
//         `test_set_text_whitespace_on_existing_text_child_leaves_empty_residual`. That test is
//         REPLACED (not silently dropped) by `test_set_text_repeated_empty_reverts_to_zero_
//         children`, asserting the CORRECT, upstream-matching outcome: `child_count()` reverts to
//         `0`, identical to calling `set_text(doc, id, "")` on a childless element, because
//         `SetInnerRML` clears UNCONDITIONALLY before ever inspecting whether the new `rml` is
//         empty -- there is no longer a "residual" case, and no longer a divergence to document.
//
//     RETURNS `false` when: `id` does not resolve (empty id, or no element with that id anywhere
//     in `doc.body()`'s subtree -- `find_by_id`'s own empty-id-always-misses rule, dom_tree.hpp,
//     applies unchanged); OR `Element::append_child` rejects the new `Text` with
//     `AppendOutcome::RejectedDepthCeiling` (the target element is already at `kMaxElementDepth` --
//     dom_tree.hpp's own existing, independent hardening ceiling, unrelated to this closure).
//     Returns `true` in every other case, INCLUDING when `text` is empty or whitespace-only.
//
//     ⚠️ ON THE `RejectedDepthCeiling` CASE SPECIFICALLY, AND WHY IT IS NOT A DATA-LOSS RISK
//     DESPITE THE CLEAR-THEN-APPEND ORDER: `RejectedDepthCeiling` can only happen when the TARGET
//     element's own `depth()` already equals `kMaxElementDepth` -- but `append_child`'s depth
//     check (`depth_ + 1 > kMaxElementDepth`) is a function of the PARENT's depth alone, applied
//     identically to EVERY child that parent has ever been offered. An element sitting exactly at
//     the ceiling can therefore never have successfully adopted so much as a first child (the very
//     first append attempt would already have hit `depth_ + 1 > kMaxElementDepth`) -- so
//     `clear_children()` on such an element is GUARANTEED to be a no-op (`0` removed) before the
//     doomed append is even attempted. There is no scenario where real content is destroyed and
//     then not replaced; a `RejectedDepthCeiling` target is, and always was, childless. Pinned by
//     `test_set_text_depth_ceiling_rejection_on_always_childless_element`.
//
//     DUPLICATE IDS: same policy as `Element::find_by_id` (dom_tree.hpp's own "Lookup policy"
//     paragraph, corpus-measured, e.g. `id="ctrl_ascii"` x4) -- the FIRST element in pre-order
//     (self, then children in source order, depth-first) is the one mutated; every other element
//     sharing that `id` is untouched. Pinned by `test_duplicate_id_first_preorder_wins_for_all_
//     three_ops` in `dom_api_sanity.cpp`.
// PT: Substitui o conteúdo de texto de um elemento por id -- REPLICA o
//     `Rml::Element::SetInnerRML` exatamente (`examples/RmlUi/Source/Core/Element.cpp:1166-1176`):
//     `Element::clear_children()` incondicional PRIMEIRO (dom_tree.hpp, UIX-REMOVE-CHILD),
//     independente do que havia antes ou de o `text` novo ser vazio, DEPOIS `Element::append_child`
//     soma um único `Text` novo -- que ELE MESMO já produz o resultado correto "zero filhos" pra um
//     `text` vazio/só-whitespace via seu próprio filtro de existência, batendo exatamente com o
//     próprio filtro `only_white_space` do `Factory::InstanceElementText`
//     (`examples/RmlUi/Source/Core/Factory.cpp:330-341`). `text` é guardado byte-verbatim (sem
//     escape, sem tratamento de entidade -- esta camada não tem noção nenhuma de risco de injeção
//     de markup; esse hardening, pela `ui_layer.hpp` linhas 886-895, é trabalho da PRÓPRIA fachada
//     com base em RmlUi quando ela eventualmente envolver esta chamada, não desta camada
//     só-de-DOM).
//
//     🟢 ISTO SUBSTITUI A IMPLEMENTAÇÃO ORIGINAL DA S4 (`f15f1f8`), QUE TRATAVA TRÊS FORMAS
//     ESPECIAIS (zero filhos / exatamente um filho Text / qualquer outra coisa) PORQUE O
//     `dom_tree.hpp` NÃO TINHA PRIMITIVO DE REMOÇÃO NENHUM. A UIX-REMOVE-CHILD (RMLX-1,
//     2026-08-05) fechou essa lacuna somando `Element::remove_child`/`Element::clear_children`;
//     esta função agora tem exatamente UM caminho de código, pra toda forma que a antiga divisão em
//     três tinha que distinguir:
//       - zero filhos antes: `clear_children()` não remove nada (`0`), comportamentalmente idêntico
//         à antiga forma (a).
//       - um único filho `Text` (o caso "sobrescrever"): `clear_children()` DESTRÓI aquele objeto
//         `Text`; o append que segue constrói um genuinamente NOVO. 🔴 **Isto é uma MUDANÇA DE
//         COMPORTAMENTO deliberada e documentada em relação ao contrato antigo**: a antiga forma
//         (b) garantia "a MESMA identidade de objeto Node/Text... preservada através da chamada"
//         (mutação no lugar via `Text::set_content`); essa garantia SUMIU. Isto bate exatamente com
//         o upstream (`RemoveChild` destrói o nó antigo, `Factory::InstanceElementText` constrói um
//         novo) e nenhum chamador deste codebase hoje segura um ponteiro observador
//         `Text*`/`Node*` através de uma chamada `set_text` (`grep -rn set_text glintfx/tests` --
//         todo ponto de chamada ou descarta o retorno ou re-resolve por id depois), então isto é um
//         fechamento seguro, não uma regressão silenciosa -- mas É novo, e está anunciado aqui em
//         vez de deixado implícito.
//       - 2+ filhos de qualquer tipo, ou um único filho não-`Text` (a antiga forma (c), 🔴 "a
//         lacuna que carrega peso"): antes RECUSAVA (`false`, árvore intocada). Agora TEM SUCESSO,
//         exatamente como o upstream -- todo filho existente é destruído, o `Text` novo (se
//         sobreviver ao filtro de whitespace) é o ÚNICO filho depois. O
//         `test_set_text_element_with_non_text_or_multiple_children_refused` (o teste que FIXAVA a
//         recusa) é SUBSTITUÍDO por
//         `test_set_text_element_with_non_text_or_multiple_children_replaces_all` -- ver o próprio
//         comentário deste teste no `dom_api_sanity.cpp` pra por que o nome antigo sumiu, não só o
//         corpo dele.
//       - `set_text(doc, id, "")` num elemento que já guarda um único filho `Text`: antes deixava
//         um RESIDUAL `Text` VAZIO (`child_count()` continuava `1`) -- a ÚNICA divergência do
//         upstream que o próprio doc-comment anterior desta fatia nomeava explicitamente, fixada
//         por `test_set_text_whitespace_on_existing_text_child_leaves_empty_residual`. Esse teste é
//         SUBSTITUÍDO (não silenciosamente apagado) por
//         `test_set_text_repeated_empty_reverts_to_zero_children`, afirmando o resultado CORRETO,
//         batendo com o upstream: `child_count()` reverte pra `0`, idêntico a chamar
//         `set_text(doc, id, "")` num elemento sem filhos, porque o `SetInnerRML` limpa
//         INCONDICIONALMENTE antes mesmo de inspecionar se o `rml` novo é vazio -- não existe mais
//         caso "residual", e não existe mais divergência pra documentar.
//
//     RETORNA `false` quando: `id` não resolve (id vazio, ou nenhum elemento com esse id em lugar
//     nenhum da subárvore de `doc.body()` -- a própria regra "id vazio sempre erra" do
//     `find_by_id`, dom_tree.hpp, vale sem mudança); OU `Element::append_child` rejeita o `Text`
//     novo com `AppendOutcome::RejectedDepthCeiling` (o elemento-alvo já está em
//     `kMaxElementDepth` -- o próprio teto de hardening já existente e independente do
//     dom_tree.hpp, sem relação com este fechamento). Retorna `true` em todo outro caso, INCLUINDO
//     quando `text` é vazio ou só-whitespace.
//
//     ⚠️ SOBRE O CASO `RejectedDepthCeiling` ESPECIFICAMENTE, E POR QUE NÃO É RISCO DE PERDA DE
//     DADO APESAR DA ORDEM CLEAR-DEPOIS-APPEND: `RejectedDepthCeiling` só consegue acontecer quando
//     a própria `depth()` do elemento-alvo já é igual a `kMaxElementDepth` -- mas a checagem de
//     profundidade do `append_child` (`depth_ + 1 > kMaxElementDepth`) é função só da profundidade
//     do PAI, aplicada de forma idêntica a TODO filho que aquele pai algum dia recebeu. Um elemento
//     sentado exatamente no teto portanto NUNCA conseguiu adotar sequer um primeiro filho (a
//     própria primeira tentativa de append já teria batido em `depth_ + 1 > kMaxElementDepth`) --
//     então `clear_children()` num elemento desses é GARANTIDAMENTE um no-op (`0` removido) antes
//     mesmo do append fadado ser tentado. Não existe cenário em que conteúdo real é destruído e
//     depois não substituído; um alvo `RejectedDepthCeiling` é, e sempre foi, sem filhos. Fixado
//     por `test_set_text_depth_ceiling_rejection_on_always_childless_element`.
//
//     IDS DUPLICADOS: mesma política do `Element::find_by_id` (o próprio parágrafo "Lookup
//     policy" do dom_tree.hpp, medido no corpus, ex. `id="ctrl_ascii"` x4) -- o PRIMEIRO elemento
//     em pré-ordem (o próprio nó, depois filhos em ordem-fonte, profundidade primeiro) é o que é
//     mutado; todo outro elemento compartilhando aquele `id` fica intocado. Fixado por
//     `test_duplicate_id_first_preorder_wins_for_all_three_ops` no `dom_api_sanity.cpp`.
bool set_text(Document& doc, std::string_view id, std::string text);

// EN: Add a single CSS class token to an element by id -- delegates to `Element::add_class`
//     (dom_tree.hpp) after resolving `id`.
//
//     RETURNS `true` whenever `id` resolves to an element AND `cls` is a structurally valid single
//     token (non-empty, no embedded whitespace -- the SAME 4-character set `Element::add_class`
//     itself checks, space/`\t`/`\n`/`\r`), REGARDLESS of whether `cls` was already present. This
//     is the EXISTING facade contract (`ui_layer.hpp` lines 925-933's own guard shape, mirrored by
//     `remove_class`'s "still returns `true`" note at lines 936-945), deliberately reconciling two
//     different booleans: `Element::add_class` itself returns `false` for an ALREADY-PRESENT class
//     (a caller-observable "nothing changed" signal, dom_tree.hpp's own comment: "false if cls was
//     already present -- caller-observable dedup signal") -- this function does NOT forward that
//     bit. It calls `Element::add_class` for its SIDE EFFECT only and reports `true` as long as
//     the call was well-formed (id found, cls valid), matching what a caller of the FACADE (not of
//     `dom_tree.hpp` directly) actually needs to know: "did my class end up applied", not "did the
//     set change size". Idempotence in BOTH directions -- a repeat `add_class` with the same
//     `cls`, and a `remove_class` on a `cls` never present -- is pinned by
//     `test_add_class_roundtrip_and_idempotent`/`test_remove_class_roundtrip_and_idempotent` in
//     `dom_api_sanity.cpp`, proving the class SET itself stays correct (no duplicate, no
//     phantom-negative-count crash) even though the return value alone would not reveal that.
//
//     RETURNS `false` when `id` does not resolve (same empty-id-always-misses rule as `set_text`
//     above) OR `cls` is structurally invalid (empty, or contains any of the 4 whitespace
//     characters) -- checked BEFORE the id lookup, same guard ORDER `ui_layer.hpp`'s own
//     `add_class`/`remove_class` doc-comments describe for the RmlUi-backed facade (cls-shape
//     first, then id). An invalid `cls` never reaches `Element::add_class` at all, so this
//     function's own pre-check is the ONLY place that can reject it -- `Element::add_class`'s own
//     `false` return for an invalid token is therefore never actually observed by this wrapper (it
//     is a redundant second line of defense inside `dom_tree.hpp`, not a case this function needs
//     to distinguish from "already present").
//
//     DUPLICATE IDS: same "first pre-order match" policy as `set_text` above -- see that
//     function's own doc-comment.
// PT: Soma um único token de classe CSS a um elemento por id -- delega ao `Element::add_class`
//     (dom_tree.hpp) depois de resolver `id`.
//
//     RETORNA `true` sempre que `id` resolve pra um elemento E `cls` é um token único
//     estruturalmente válido (não-vazio, sem whitespace embutido -- o MESMO conjunto de 4
//     caracteres que o próprio `Element::add_class` checa, espaço/`\t`/`\n`/`\r`), INDEPENDENTE de
//     `cls` já estar presente. Este é o CONTRATO EXISTENTE da fachada (a própria forma de guard das
//     linhas 925-933 da `ui_layer.hpp`, espelhada pela nota "ainda retorna `true`" do
//     `remove_class` nas linhas 936-945), reconciliando deliberadamente dois booleanos diferentes:
//     o próprio `Element::add_class` retorna `false` pra uma classe JÁ-PRESENTE (um sinal
//     observável-pelo-chamador de "nada mudou", o próprio comentário do dom_tree.hpp: "false se
//     cls já estava presente -- sinal de dedup observável pelo chamador") -- esta função NÃO
//     encaminha esse bit. Ela chama `Element::add_class` só pelo EFEITO COLATERAL e reporta `true`
//     desde que a chamada tenha sido bem-formada (id achado, cls válido), batendo com o que um
//     chamador da FACHADA (não do `dom_tree.hpp` diretamente) realmente precisa saber: "minha
//     classe terminou aplicada", não "o conjunto mudou de tamanho". Idempotência nas DUAS direções
//     -- um `add_class` repetido com o mesmo `cls`, e um `remove_class` sobre um `cls` nunca
//     presente -- é fixada por
//     `test_add_class_roundtrip_and_idempotent`/`test_remove_class_roundtrip_and_idempotent` no
//     `dom_api_sanity.cpp`, provando que o próprio CONJUNTO de classe fica correto (sem
//     duplicata, sem contagem-fantasma-negativa) mesmo que o valor de retorno sozinho não revelasse
//     isso.
//
//     RETORNA `false` quando `id` não resolve (mesma regra id-vazio-sempre-erra do `set_text`
//     acima) OU `cls` é estruturalmente inválido (vazio, ou contém qualquer um dos 4 caracteres de
//     whitespace) -- checado ANTES do lookup de id, mesma ORDEM de guard que os próprios
//     doc-comments de `add_class`/`remove_class` da `ui_layer.hpp` descrevem pra fachada com base
//     em RmlUi (forma-de-cls primeiro, depois id). Um `cls` inválido nunca chega no
//     `Element::add_class` de jeito nenhum, então a pré-checagem desta própria função é o ÚNICO
//     lugar que consegue rejeitá-lo -- o próprio `false` de retorno do `Element::add_class` pra um
//     token inválido portanto nunca é de fato observado por este wrapper (é uma segunda linha de
//     defesa redundante dentro do `dom_tree.hpp`, não um caso que esta função precise distinguir de
//     "já presente").
//
//     IDS DUPLICADOS: mesma política "primeiro casamento em pré-ordem" do `set_text` acima -- ver
//     o próprio doc-comment daquela função.
bool add_class(Document& doc, std::string_view id, std::string_view cls);

// EN: Remove a single CSS class token from an element by id -- delegates to `Element::
//     remove_class` (dom_tree.hpp) after resolving `id`. Mirrors `add_class`'s own contract
//     exactly (see its doc-comment above for the full argument): `true` whenever `id` resolves and
//     `cls` is a structurally valid single token, REGARDLESS of whether `cls` was actually present
//     to remove (removing an absent class is a safe, idempotent no-op that still reports success --
//     `ui_layer.hpp` lines 936-945's own stated contract for the RmlUi-backed facade, reconciled
//     against `Element::remove_class`'s own `false`-if-absent boolean the same way `add_class`
//     reconciles `Element::add_class`'s `false`-if-already-present boolean). `false` only for an
//     unresolved `id` or a structurally invalid `cls` (checked first, same order as `add_class`).
// PT: Remove um único token de classe CSS de um elemento por id -- delega ao `Element::
//     remove_class` (dom_tree.hpp) depois de resolver `id`. Espelha exatamente o próprio contrato
//     do `add_class` (ver o doc-comment dele acima pro argumento completo): `true` sempre que `id`
//     resolve e `cls` é um token único estruturalmente válido, INDEPENDENTE de `cls` realmente
//     estar presente pra remover (remover uma classe ausente é um no-op seguro e idempotente que
//     ainda reporta sucesso -- o próprio contrato declarado nas linhas 936-945 da `ui_layer.hpp`
//     pra fachada com base em RmlUi, reconciliado contra o próprio booleano
//     `false`-se-ausente do `Element::remove_class` do mesmo jeito que o `add_class` reconcilia o
//     booleano `false`-se-já-presente do `Element::add_class`). `false` só pra um `id` não
//     resolvido ou um `cls` estruturalmente inválido (checado primeiro, mesma ordem do
//     `add_class`).
bool remove_class(Document& doc, std::string_view id, std::string_view cls);

} // namespace glintfx::uix
