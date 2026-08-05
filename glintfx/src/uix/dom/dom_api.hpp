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
//       (2) `set_text` on an element that already has children is where this slice's real
//           boundary sits -- see `set_text`'s own doc-comment, "THE LOAD-BEARING GAP" paragraph,
//           for the full argument and the concrete `dom_tree.hpp` primitive a future wave needs to
//           close it. This is NOT a guess papered over; it is a documented, tested boundary this
//           slice's own brief explicitly asked to be surfaced rather than worked around.
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
//       (2) `set_text` num elemento que já tem filhos é onde a fronteira real desta fatia mora --
//           ver o próprio doc-comment do `set_text`, parágrafo "A LACUNA QUE CARREGA PESO", pro
//           argumento completo e o primitivo concreto do `dom_tree.hpp` que uma onda futura
//           precisa pra fechar isto. Isto NÃO é um chute disfarçado; é uma fronteira documentada e
//           testada que o próprio briefing desta fatia pediu explicitamente pra ser exposta em vez
//           de contornada.
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

// EN: Replace an element's text content by id. `text` is stored byte-verbatim (no escaping, no
//     entity handling -- this layer has no notion of markup injection risk; that hardening, per
//     `ui_layer.hpp` lines 886-895, is the RmlUi-backed facade's own job when it eventually wraps
//     this call, not this DOM-only layer's).
//
//     RETURNS `false` when: `id` does not resolve (empty id, or no element with that id anywhere
//     in `doc.body()`'s subtree -- `find_by_id`'s own empty-id-always-misses rule, dom_tree.hpp,
//     applies unchanged); OR the target element's existing children are not in the ONE SHAPE this
//     slice can safely rewrite (see "THE LOAD-BEARING GAP" below). Returns `true` in every other
//     case, INCLUDING when `text` is empty or whitespace-only (see "WHITESPACE INPUT" below).
//
//     DUPLICATE IDS: same policy as `Element::find_by_id` (dom_tree.hpp's own "Lookup policy"
//     paragraph, corpus-measured, e.g. `id="ctrl_ascii"` x4) -- the FIRST element in pre-order
//     (self, then children in source order, depth-first) is the one mutated; every other element
//     sharing that `id` is untouched. Pinned by `test_duplicate_id_first_preorder_wins_for_all_
//     three_ops` in `dom_api_sanity.cpp`.
//
//     THE THREE SHAPES THIS FUNCTION HANDLES, AND WHY EACH IS THE RIGHT CHOICE GIVEN WHAT S2's
//     `dom_tree.hpp` ACTUALLY EXPOSES (no `remove_child`/`clear_children`/`replace_child` exists
//     anywhere in that header -- its own header comment says so explicitly: "S3 ... never needs
//     anything but `append_child`; ... left for S4 to add in the shape it actually turns out to
//     need, not guessed at here". This function is that "actually turns out to need" moment, and
//     what it turns out to need is a REMOVAL primitive S2 does not have):
//
//       (a) ZERO existing children -- the common case (every real caller in this codebase today,
//           `grep -rn set_text glintfx/tests`, targets a freshly-parsed, still-empty element:
//           `"text-target"`, `"txt"`, `"grower"`). `text` is appended as a single new `Text` child
//           via `Element::append_child` -- which ITSELF filters an empty/whitespace-only `text`
//           down to zero children (dom_tree.hpp's own existence-filter invariant), so this
//           function does not special-case that up front; it just tries the append and reads back
//           `AppendResult::outcome`. `AppendOutcome::RejectedDepthCeiling` (a genuinely-failing
//           append, `RMLX-1/S2`'s own fail-high hardening) is the ONE way this shape can return
//           `false`; `Appended` and `FilteredWhitespaceText` both mean "the write happened exactly
//           as requested" and return `true`.
//
//       (b) EXACTLY ONE existing child, and it is a `Text` node -- the "overwrite" case a caller
//           gets by calling `set_text` twice on the same id (`domrw_sanity.cpp`'s own
//           happy-path/nullptr/malicious-text chain against the RmlUi-backed engine is the exact
//           usage shape this mirrors). `dom_tree.hpp` gives NO way to detach that existing `Text`
//           node from `children()` (the accessor is `const`-returning, and there is no
//           `remove_child`), but it DOES give a way to MUTATE it in place: `Text::set_content`.
//           `Element::children()` returns a `const vector<unique_ptr<Node>>&`, but
//           `unique_ptr<T>::get() const` itself returns `T*` (NOT `const T*` -- constness does not
//           propagate through a `unique_ptr`'s pointee, only through the `unique_ptr` object
//           itself), so `as_text(el->children().front().get())` legitimately yields a mutable
//           `Text*` without any `const_cast` -- this function relies on that standard-library fact,
//           not on casting anything away. The SAME `Node`/`Text` object identity is preserved
//           across the call (pinned by `test_set_text_overwrites_existing_sole_text_child`'s own
//           pointer-identity check) -- this is a real in-place mutation, not a
//           destroy-and-reappend, which matters because a caller holding an observer pointer into
//           this tree (mirroring `AppendResult::node`'s own contract) must not have it silently
//           invalidated by an ordinary overwrite.
//
//       (c) ANYTHING ELSE (two or more children of any kind, or a single child that is an
//           `Element` rather than a `Text`) -- 🔴 **THE LOAD-BEARING GAP.** Upstream, `Rml::
//           Element::SetInnerRML` unconditionally removes EVERY existing child before instancing
//           the new text (`examples/RmlUi/Source/Core/Element.cpp:1170-1172`, `while ((int)
//           children.size() > num_non_dom_children) RemoveChild(children.front().get());`) --
//           i.e. `set_text` upstream ALWAYS succeeds and ALWAYS fully replaces the subtree,
//           regardless of what was there before. This function CANNOT replicate that: `dom_tree.
//           hpp`'s `Element` has no removal/replacement primitive at all (`append_child` is
//           documented as the ONLY mutator S2 built, deliberately, per its own header comment).
//           Rather than fake a workaround that would silently diverge from upstream in a way
//           nobody would notice until `S7`'s dump-parity work (e.g. appending a SECOND `Text`
//           child next to the old one, which would neither match "replace" nor "append" semantics
//           cleanly, and would corrupt shape (a)/(b)'s own invariant that a `set_text`-managed
//           element carries AT MOST one `Text` child), this function REFUSES (`false`) and leaves
//           the tree byte-for-byte untouched -- fail-high, this project's own "AUD-TEC-5"
//           discipline, applied to a case S2's surface genuinely cannot serve rather than to
//           invalid input. **This is reported to the líder as the single most load-bearing finding
//           of this slice**: closing it needs ONE new `dom_tree.hpp` primitive in a future wave
//           (a `remove_child`/`clear_children`, or a `replace_children` shaped for exactly this
//           call site) -- deliberately not added here, since S4's own brief says "não toque em
//           dom_tree.{hpp,cpp} ... reporte" and dom_tree.hpp itself already earmarked this exact
//           decision for "S4 to add in the shape it actually turns out to need". Pinned by
//           `test_set_text_element_with_non_text_or_multiple_children_refused`.
//
//     WHITESPACE INPUT (the aceite's explicit question: "o que deve acontecer" when `text` is
//     whitespace-only): shape (a) answers it "for free" via `append_child`'s own existence
//     filter -- `set_text(doc, "empty-span", "   ")` returns `true` and leaves the element with
//     ZERO children, matching upstream EXACTLY (`Factory::InstanceElementText`,
//     `examples/RmlUi/Source/Core/Factory.cpp:338-341`: "If this text node only contains
//     white-space we don't want to construct it" -- `only_white_space` -> `return true` with no
//     child instanced, the identical "success, zero children" outcome). Shape (b) does NOT have
//     this luxury (see (b) above -- `Text::set_content` has no filter, unlike `append_child`): a
//     REPEATED `set_text(doc, id, "")` on an element that already holds a sole `Text` child leaves
//     that child PRESENT with EMPTY content (`child_count()` stays `1`) rather than reverting to
//     zero children like the empty-element case does -- a second, narrower instance of the same
//     load-bearing gap in (c), since reverting to zero children there WOULD require the same
//     missing removal primitive. Pinned (both directions) by
//     `test_set_text_whitespace_only_on_empty_element_filtered` and
//     `test_set_text_whitespace_on_existing_text_child_leaves_empty_residual`.
// PT: Substitui o conteúdo de texto de um elemento por id. `text` é guardado byte-verbatim (sem
//     escape, sem tratamento de entidade -- esta camada não tem noção nenhuma de risco de injeção
//     de markup; esse hardening, pela `ui_layer.hpp` linhas 886-895, é trabalho da PRÓPRIA fachada
//     com base em RmlUi quando ela eventualmente envolver esta chamada, não desta camada
//     só-de-DOM).
//
//     RETORNA `false` quando: `id` não resolve (id vazio, ou nenhum elemento com esse id em
//     lugar nenhum da subárvore de `doc.body()` -- a própria regra "id vazio sempre erra" do
//     `find_by_id`, dom_tree.hpp, vale sem mudança); OU os filhos existentes do elemento-alvo não
//     estão numa das FORMAS que esta fatia consegue reescrever com segurança (ver "A LACUNA QUE
//     CARREGA PESO" abaixo). Retorna `true` em todo outro caso, INCLUINDO quando `text` é vazio ou
//     só-whitespace (ver "INPUT DE WHITESPACE" abaixo).
//
//     IDS DUPLICADOS: mesma política do `Element::find_by_id` (o próprio parágrafo "Lookup
//     policy" do dom_tree.hpp, medido no corpus, ex. `id="ctrl_ascii"` x4) -- o PRIMEIRO elemento
//     em pré-ordem (o próprio nó, depois filhos em ordem-fonte, profundidade primeiro) é o que é
//     mutado; todo outro elemento compartilhando aquele `id` fica intocado. Fixado por
//     `test_duplicate_id_first_preorder_wins_for_all_three_ops` no `dom_api_sanity.cpp`.
//
//     AS TRÊS FORMAS QUE ESTA FUNÇÃO TRATA, E POR QUE CADA UMA É A ESCOLHA CERTA DADO O QUE O
//     `dom_tree.hpp` DA S2 REALMENTE EXPÕE (nenhum `remove_child`/`clear_children`/
//     `replace_child` existe em lugar nenhum daquele header -- o próprio comentário de cabeçalho
//     dele diz isso explicitamente: "a S3 ... nunca precisa de nada além de `append_child`; ...
//     fica pra S4 somar na forma que ela realmente precisar, não chutado aqui". Esta função é esse
//     momento de "realmente precisar", e o que ela realmente precisa é um primitivo de REMOÇÃO que
//     a S2 não tem):
//
//       (a) ZERO filhos existentes -- o caso comum (todo chamador real neste codebase hoje, `grep
//           -rn set_text glintfx/tests`, mira um elemento recém-parseado, ainda vazio:
//           `"text-target"`, `"txt"`, `"grower"`). `text` é somado como um único `Text` novo via
//           `Element::append_child` -- que ELA MESMA filtra `text` vazio/só-whitespace até zero
//           filhos (o próprio invariante de filtro-de-existência do dom_tree.hpp), então esta
//           função não trata isso como caso especial antecipado; só tenta o append e lê de volta
//           `AppendResult::outcome`. `AppendOutcome::RejectedDepthCeiling` (um append genuinamente
//           falho, o próprio hardening fail-high da `RMLX-1/S2`) é a ÚNICA forma desta forma
//           retornar `false`; `Appended` e `FilteredWhitespaceText` os dois significam "a escrita
//           aconteceu exatamente como pedido" e retornam `true`.
//
//       (b) EXATAMENTE UM filho existente, e ele é um nó `Text` -- o caso "sobrescrever" que um
//           chamador obtém ao chamar `set_text` duas vezes no mesmo id (a própria cadeia
//           happy-path/nullptr/texto-malicioso do `domrw_sanity.cpp` contra o engine com base em
//           RmlUi é exatamente a forma de uso que isto espelha). O `dom_tree.hpp` NÃO dá jeito
//           nenhum de desanexar aquele `Text` existente de `children()` (o acessador retorna
//           `const`, e não existe `remove_child`), mas DÁ um jeito de MUTÁ-LO no lugar:
//           `Text::set_content`. `Element::children()` retorna um `const vector<unique_ptr<Node>>&`,
//           mas o próprio `unique_ptr<T>::get() const` retorna `T*` (NÃO `const T*` --
//           constância não se propaga pelo apontado de um `unique_ptr`, só pelo próprio objeto
//           `unique_ptr`), então `as_text(el->children().front().get())` legitimamente rende um
//           `Text*` mutável sem `const_cast` nenhum -- esta função conta com esse fato padrão da
//           biblioteca, não com tirar constância de nada. A MESMA identidade de objeto
//           `Node`/`Text` é preservada através da chamada (fixada pela própria checagem de
//           identidade-de-ponteiro do `test_set_text_overwrites_existing_sole_text_child`) -- isto
//           é mutação-no-lugar de verdade, não destruir-e-re-somar, o que importa porque um
//           chamador segurando um ponteiro observador nesta árvore (espelhando o próprio contrato
//           de `AppendResult::node`) não pode tê-lo invalidado em silêncio por uma sobrescrita
//           comum.
//
//       (c) QUALQUER OUTRA COISA (dois ou mais filhos de qualquer tipo, ou um único filho que é um
//           `Element` em vez de `Text`) -- 🔴 **A LACUNA QUE CARREGA PESO.** No upstream, o próprio
//           `Rml::Element::SetInnerRML` remove INCONDICIONALMENTE todo filho existente antes de
//           instanciar o texto novo (`examples/RmlUi/Source/Core/Element.cpp:1170-1172`, `while
//           ((int)children.size() > num_non_dom_children) RemoveChild(children.front().get());`)
//           -- ou seja, o `set_text` upstream SEMPRE tem sucesso e SEMPRE substitui a subárvore
//           por completo, independente do que havia antes. Esta função NÃO CONSEGUE replicar isso:
//           o `Element` do `dom_tree.hpp` não tem primitivo nenhum de remoção/substituição
//           (`append_child` é documentado como o ÚNICO mutador que a S2 construiu, deliberadamente,
//           pelo próprio comentário de cabeçalho dela). Em vez de forjar um contorno que
//           divergiria do upstream em silêncio de um jeito que ninguém notaria até o trabalho de
//           paridade-de-dump da `S7` (ex.: somar um SEGUNDO `Text` filho ao lado do antigo, que não
//           casaria nem com semântica de "substituir" nem de "somar" de forma limpa, e corromperia
//           o próprio invariante das formas (a)/(b) de que um elemento gerido por `set_text` carrega
//           NO MÁXIMO um filho `Text`), esta função RECUSA (`false`) e deixa a árvore intocada
//           byte-por-byte -- fail-high, a própria disciplina "AUD-TEC-5" deste projeto, aplicada a
//           um caso que a superfície da S2 genuinamente não consegue servir, não a input inválido.
//           **Isto é reportado ao líder como o achado mais carregado-de-peso desta fatia**: fechar
//           isto precisa de UM novo primitivo no `dom_tree.hpp` numa onda futura (um
//           `remove_child`/`clear_children`, ou um `replace_children` desenhado exatamente pra este
//           ponto de chamada) -- deliberadamente não somado aqui, já que o próprio briefing da S4
//           diz "não toque em dom_tree.{hpp,cpp} ... reporte" e o próprio dom_tree.hpp já reservou
//           exatamente esta decisão pra "S4 somar na forma que ela realmente precisar". Fixado por
//           `test_set_text_element_with_non_text_or_multiple_children_refused`.
//
//     INPUT DE WHITESPACE (a pergunta explícita do aceite: "o que deve acontecer" quando `text` é
//     só-whitespace): a forma (a) responde "de graça" via o próprio filtro de existência do
//     `append_child` -- `set_text(doc, "empty-span", "   ")` retorna `true` e deixa o elemento com
//     ZERO filhos, batendo EXATAMENTE com o upstream (`Factory::InstanceElementText`,
//     `examples/RmlUi/Source/Core/Factory.cpp:338-341`: "If this text node only contains
//     white-space we don't want to construct it" -- `only_white_space` -> `return true` sem filho
//     nenhum instanciado, o MESMO resultado "sucesso, zero filhos"). A forma (b) NÃO tem esse luxo
//     (ver (b) acima -- `Text::set_content` não tem filtro, diferente de `append_child`): um
//     `set_text(doc, id, "")` REPETIDO num elemento que já guarda um único filho `Text` deixa
//     aquele filho PRESENTE com conteúdo VAZIO (`child_count()` continua `1`) em vez de reverter
//     pra zero filhos como o caso de elemento-vazio faz -- uma segunda instância, mais estreita, da
//     mesma lacuna carregada-de-peso do (c), já que reverter pra zero filhos ali TAMBÉM precisaria
//     do mesmo primitivo de remoção que falta. Fixado (nas duas direções) por
//     `test_set_text_whitespace_only_on_empty_element_filtered` e
//     `test_set_text_whitespace_on_existing_text_child_leaves_empty_residual`.
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
