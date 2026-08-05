// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6a -- dumper over the REAL Rml::ElementDocument tree, emitting the format
//     defined in `docs/uix-dom.md` (the SOLE contract this file implements; do not add any
//     behaviour not written there without editing that document first). This is one of the
//     TWO independent, oracle-half implementations `RMLX-1` needs (`S6b` walks glintfx's own
//     `src/uix/dom/` tree instead) -- by design this file's author has NOT read `S6b`'s source,
//     and `S6b`'s author must not read this one either (see uix-dom.md's own "Why this document
//     exists" section for why a shared author/reader would poison the differential oracle).
//
//     Confinement: this header/its .cpp live in `glintfx/src/rml/` -- the ONLY place in this
//     repo permitted to `#include <RmlUi/...>` (`tools/check_rml_whitelist.sh`). The public
//     surface below forward-declares `Rml::ElementDocument` only (same pImpl-adjacent
//     discipline as `bootstrap.hpp`'s `Rml::Context*`/`Rml::SystemInterface*`), so a caller
//     OUTSIDE `src/rml/` could technically link against `dump_document()` without itself
//     needing an `Rml::` token -- in practice this wave's only caller is this file's own test
//     executables, which ALSO live inside `src/rml/` (see dom_dump_corpus_sanity.cpp /
//     dom_dump_determinism_sanity.cpp) precisely so a NEW test never has to fight
//     check (b)/(c) of the whitelist gate (both scope to `glintfx/tests/`, never
//     `glintfx/src/rml/`).
//
//     What this dumper does NOT try to do, on purpose: it does not re-implement RmlUi's own
//     whitespace-only-text-node existence filter (docs/uix-dom.md section 6a) -- that filter is
//     already applied by `Factory::InstanceElementText`
//     (`examples/RmlUi/Source/Core/Factory.cpp:330-341`) BEFORE any node this dumper can see
//     exists at all, so every child `GetChild(i)` returns is already a "surviving" child by
//     construction. `S6b`, walking its OWN from-scratch parser's tree, has to implement that
//     filter itself; `S6a` inherits it for free by definition of "the real tree". This is the
//     single biggest structural asymmetry between the two dumpers and is worth naming loudly so
//     nobody "ports" a whitespace filter into this file by habit.
//
//     🔴 SPEC ADDENDUM (2026-08-05, S6a finding, added HERE and cross-referenced from
//     docs/uix-dom.md section 7 -- not a private workaround): section 7 says to split the
//     `class` attribute's "raw value". This dumper reads that raw value via
//     `Element::GetAttribute<Rml::String>("class", "")` -- the verbatim, entity-decoded
//     attribute text as stored in the element's own attribute dictionary -- and NEVER via
//     `Element::GetClassNames()`. The two are NOT interchangeable: RmlUi's own
//     `ElementStyle::SetClassNames` (`examples/RmlUi/Source/Core/ElementStyle.cpp:580-583`)
//     splits ONLY on a literal `' '` (`StringUtilities::ExpandString(classes, class_names,
//     ' ')`), not on the 4-character whitespace set section 6/7 define -- so a class attribute
//     using a tab or newline as a separator would be tokenized DIFFERENTLY by
//     `GetClassNames()`'s own reconstruction (tab/newline stay glued to the neighbouring
//     token) than by section 7's own definition (tab/newline ARE separators). No fixture in
//     this wave's corpus exercises this (all real corpus `class=` values use plain space), so
//     this is not a divergence-ledger row (docs/uix-dom.md section 9's own "if the corpus
//     doesn't exercise it" carve-out) -- it is a spec-faithfulness decision, pinned by
//     dom_dump_determinism_sanity.cpp's F2 case (a `&#9;`-separated class list), documented here
//     so `S6b`'s author reads the SAME resolution without needing to re-derive it from RmlUi
//     source independently.
// PT: RMLX-1/S6a -- dumper sobre a árvore REAL Rml::ElementDocument, emitindo o formato
//     definido em `docs/uix-dom.md` (o ÚNICO contrato que este arquivo implementa; não somar
//     comportamento nenhum que não esteja escrito lá sem editar aquele documento primeiro). É
//     uma das DUAS implementações independentes, metade-do-oráculo, que a `RMLX-1` precisa
//     (`S6b` percorre a árvore própria da glintfx em `src/uix/dom/` em vez desta) -- de
//     propósito o autor deste arquivo NÃO leu o fonte da `S6b`, e o autor da `S6b` também não
//     pode ler este (ver a própria seção "Por que este documento existe" do uix-dom.md pro
//     motivo de um autor/leitor compartilhado envenenar o oráculo diferencial).
//
//     Confinamento: este header/seu .cpp moram em `glintfx/src/rml/` -- o ÚNICO lugar deste
//     repo com permissão de `#include <RmlUi/...>` (`tools/check_rml_whitelist.sh`). A
//     superfície pública abaixo só forward-declara `Rml::ElementDocument` (mesma disciplina
//     pImpl-adjacente do `Rml::Context*`/`Rml::SystemInterface*` de `bootstrap.hpp`), então um
//     chamador FORA de `src/rml/` poderia tecnicamente linkar contra `dump_document()` sem
//     precisar de nenhum token `Rml::` -- na prática o único chamador desta onda são os próprios
//     executáveis de teste deste arquivo, que TAMBÉM moram dentro de `src/rml/` (ver
//     dom_dump_corpus_sanity.cpp / dom_dump_determinism_sanity.cpp) precisamente para que um
//     teste NOVO nunca precise brigar com o check (b)/(c) do gate de whitelist (os dois são
//     escopados a `glintfx/tests/`, nunca a `glintfx/src/rml/`).
//
//     O que este dumper de propósito NÃO tenta fazer: não reimplementa o próprio filtro de
//     existência de nó-de-texto-só-whitespace do RmlUi (docs/uix-dom.md seção 6a) -- esse filtro
//     já é aplicado pelo `Factory::InstanceElementText`
//     (`examples/RmlUi/Source/Core/Factory.cpp:330-341`) ANTES de qualquer nó que este dumper
//     consiga ver sequer existir, então todo filho que `GetChild(i)` devolve já é "sobrevivente"
//     por construção. A `S6b`, percorrendo a árvore do PRÓPRIO parser dela, feito do zero,
//     precisa implementar esse filtro sozinha; a `S6a` o herda de graça, por definição de "a
//     árvore real". Esta é a maior assimetria estrutural entre os dois dumpers e vale nomear em
//     voz alta pra ninguém "portar" um filtro de whitespace pra este arquivo por hábito.
//
//     🔴 ADENDO DE SPEC (2026-08-05, achado da S6a, somado AQUI e referenciado pela seção 7 do
//     docs/uix-dom.md -- não um contorno privado): a seção 7 manda dividir o "valor cru" do
//     atributo `class`. Este dumper lê esse valor cru via
//     `Element::GetAttribute<Rml::String>("class", "")` -- o texto de atributo verbatim,
//     entity-decodificado, como guardado no próprio dicionário de atributos do elemento -- e
//     NUNCA via `Element::GetClassNames()`. Os dois NÃO são intercambiáveis: o próprio
//     `ElementStyle::SetClassNames` do RmlUi
//     (`examples/RmlUi/Source/Core/ElementStyle.cpp:580-583`) divide SÓ num `' '` literal
//     (`StringUtilities::ExpandString(classes, class_names, ' ')`), não no conjunto de 4
//     caracteres de whitespace que as seções 6/7 definem -- então um atributo class usando tab
//     ou newline como separador seria tokenizado DIFERENTE pela reconstrução do próprio
//     `GetClassNames()` (tab/newline ficam colados ao token vizinho) do que pela própria
//     definição da seção 7 (tab/newline SÃO separadores). Nenhuma fixture do corpus desta onda
//     exercita isto (todo `class=` real do corpus usa espaço simples), então não é uma linha do
//     divergence-ledger (o próprio carve-out "se o corpus não exercita" da seção 9 do
//     docs/uix-dom.md) -- é uma decisão de fidelidade-à-spec, presa pelo caso F2 do
//     dom_dump_determinism_sanity.cpp (uma lista de classes separada por `&#9;`), documentada
//     aqui pra o autor da `S6b` ler a MESMA resolução sem precisar rederivar do fonte do RmlUi
//     de forma independente.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <string>

// EN: Forward-declare only -- see this file's header comment above for why no other RmlUi
//     header is pulled into the PUBLIC surface (the .cpp is free to include whatever it needs;
//     it lives in the confinement zone too).
// PT: Só forward-declaration -- ver o comentário de cabeçalho deste arquivo acima pro motivo de
//     nenhum outro header do RmlUi ser puxado pra superfície PÚBLICA (o .cpp é livre pra incluir
//     o que precisar; ele também mora na zona de confinamento).
namespace Rml {
class ElementDocument;
} // namespace Rml

namespace glintfx {

// EN: Escapes ONE value-bearing field per docs/uix-dom.md section 2: `\` -> `\\`, `\n` -> `\n`
//     (two chars), `\r` -> `\r` (two chars), `\t` -> `\t` (two chars), every other byte
//     (including literal space and every UTF-8 continuation byte) passed through unchanged.
//     Implemented as a SINGLE left-to-right pass over `raw` that never re-scans its own output
//     -- this is what makes section 2's "apply in this exact order" requirement automatic
//     rather than something a caller has to get right: each INPUT byte is classified exactly
//     once and either copied verbatim or replaced by its two-character escape, so the
//     backslash introduced by escaping `\n`/`\r`/`\t` is never itself re-examined as if it were
//     new input.
// PT: Escapa UM campo que carrega valor conforme a seção 2 do docs/uix-dom.md: `\` -> `\\`,
//     `\n` -> `\n` (dois chars), `\r` -> `\r` (dois chars), `\t` -> `\t` (dois chars), todo
//     outro byte (incluindo espaço literal e toda continuação UTF-8) passa sem mudança.
//     Implementado como UMA ÚNICA passada esquerda-pra-direita sobre `raw` que nunca re-varre a
//     própria saída -- é isso que torna o requisito "aplicar nesta ordem exata" da seção 2
//     automático em vez de algo que um chamador precisa acertar: cada byte de ENTRADA é
//     classificado exatamente uma vez e ou copiado ao pé da letra ou substituído pelo escape de
//     dois caracteres, então a barra invertida introduzida ao escapar `\n`/`\r`/`\t` nunca é ela
//     mesma re-examinada como se fosse entrada nova.
std::string dom_dump_escape(const std::string& raw);

// EN: Result of scanning RAW RML SOURCE TEXT (not the parsed tree -- see docs/uix-dom.md
//     section 4 and this file's own .cpp for why the parsed `Rml::ElementDocument` cannot
//     answer this question at all) for the `<head>...</head>` span.
//     `present == false` (the default) mirrors `HEAD ABSENT`; `present == true` mirrors
//     `HEAD PRESENT`, with `raw` holding the EXACT, un-escaped, non-entity-decoded source bytes
//     from just after `<head...>`'s closing `>` to just before `</head>`'s opening `<` (or the
//     empty string for a self-closing `<head/>`).
// PT: Resultado de escanear o TEXTO-FONTE CRU do RML (não a árvore parseada -- ver a seção 4 do
//     docs/uix-dom.md e o próprio .cpp deste arquivo pro motivo do `Rml::ElementDocument`
//     parseado não conseguir responder esta pergunta de jeito nenhum) pelo trecho
//     `<head>...</head>`.
//     `present == false` (o default) espelha `HEAD ABSENT`; `present == true` espelha
//     `HEAD PRESENT`, com `raw` guardando os bytes-fonte EXATOS, não-escapados, não-entity-
//     decodificados, desde logo após o `>` de fechamento de `<head...>` até logo antes do `<`
//     de abertura de `</head>` (ou string vazia para um `<head/>` auto-fechado).
struct HeadScanResult {
  bool present = false;
  std::string raw;
};

// EN: Scans `source` (the exact bytes fed to RmlUi's parser for this document) for the
//     `<head>` span. Deliberately a PLAIN TEXT scan, independent of any parser or the
//     `Rml::ElementDocument` this TU also walks -- `<head>`'s children never become
//     `Rml::Element` objects upstream (docs/uix-dom.md section 4), so there is no tree-based
//     way to recover this span; it must come from the source bytes directly, the same way
//     `S6b`'s own lexer/parser is documented (dom_tree.hpp `HeadContent`) to have captured it
//     during ITS OWN from-scratch tokenizing pass.
//
//     Matching is case-insensitive on the tag names `head`/`/head` (RmlUi's own
//     `XMLParser::HandleElementStart` lower-cases every tag name before dispatch --
//     `StringUtilities::ToLower`, `examples/RmlUi/Source/Core/XMLParser.cpp:136` -- so a
//     source-authored `<HEAD>` is treated by upstream exactly like `<head>`; this scan mirrors
//     that), quote-aware only for the OPENING tag's own attribute values (so a `>` inside a
//     quoted attribute of `<head ...>` itself does not falsely end the tag early), and a plain
//     (non-quote-aware, non-nesting-aware) substring search for the LITERAL text `</head`
//     thereafter -- `<head>` cannot itself nest another `<head>` (RML/XML grammar), and
//     docs/uix-dom.md section 4 defines the boundary in exactly these literal terms ("up to
//     (not including) the `<` that opens `</head>`"), not in terms of a nesting-aware closing
//     match. A `<head ...guarded.../>` self-closing open tag (attribute value's own `/>` is
//     still respected as part of the quote-aware opening-tag scan) yields `{true, ""}` with no
//     further search.
//
//     No fixture in this wave's corpus (glintfx/tests/*.rml, glintfx/demos/showcase/*.rml)
//     exercises a nested `</head` collision (e.g. that literal substring appearing inside a
//     `<style>` block's own CSS comment) or a self-closing `<head/>` -- both code paths exist
//     and are unit-proven (dom_dump_determinism_sanity.cpp) but are not corpus-exercised
//     findings, just documented implementation choices consistent with docs/uix-dom.md's own
//     "if the corpus doesn't exercise it, don't invent handling for it" posture (section 4's
//     own `<head>`-attribute-non-capture rule is the precedent).
// PT: Escaneia `source` (os bytes exatos alimentados ao parser do RmlUi pra este documento) em
//     busca do trecho `<head>`. De propósito um scan de TEXTO PURO, independente de qualquer
//     parser ou do `Rml::ElementDocument` que esta TU também percorre -- os filhos de `<head>`
//     nunca viram objeto `Rml::Element` nenhum upstream (seção 4 do docs/uix-dom.md), então não
//     há jeito baseado-em-árvore de recuperar este trecho; tem que vir direto dos bytes-fonte,
//     do mesmo jeito que o próprio lexer/parser da `S6b` (dom_tree.hpp `HeadContent`) está
//     documentado a ter capturado durante a PRÓPRIA passada de tokenização feita do zero.
//
//     Casamento é insensível a caixa nos nomes de tag `head`/`/head` (o próprio
//     `XMLParser::HandleElementStart` do RmlUi dobra todo nome de tag pra minúscula antes do
//     despacho -- `StringUtilities::ToLower`,
//     `examples/RmlUi/Source/Core/XMLParser.cpp:136` -- então um `<HEAD>` escrito na fonte é
//     tratado pelo upstream exatamente como `<head>`; este scan espelha isso), consciente de
//     aspas só pros valores de atributo da PRÓPRIA tag de abertura (pra um `>` dentro de um
//     atributo entre aspas de `<head ...>` não fechar a tag cedo demais por engano), e uma
//     busca de substring simples (sem consciência de aspas, sem consciência de aninhamento)
//     pelo texto LITERAL `</head` daí em diante -- `<head>` não pode aninhar outro `<head>`
//     (gramática RML/XML), e a seção 4 do docs/uix-dom.md define a fronteira exatamente nestes
//     termos literais ("até (sem incluir) o `<` que abre `</head>`"), não em termos de um
//     casamento de fechamento consciente de aninhamento. Uma tag de abertura auto-fechada
//     `<head ...guardado.../>` (o próprio `/>` de um valor de atributo entre aspas continua
//     respeitado como parte do scan consciente-de-aspas da tag de abertura) devolve
//     `{true, ""}` sem busca adicional.
//
//     Nenhuma fixture do corpus desta onda (glintfx/tests/*.rml, glintfx/demos/showcase/*.rml)
//     exercita uma colisão `</head` aninhada (ex.: esse substring literal aparecendo dentro do
//     próprio comentário CSS de um bloco `<style>`) nem um `<head/>` auto-fechado -- os dois
//     caminhos de código existem e são provados por teste unitário
//     (dom_dump_determinism_sanity.cpp), mas não são achados exercitados pelo corpus, só
//     escolhas de implementação documentadas, consistentes com a própria postura "se o corpus
//     não exercita, não inventa tratamento" do docs/uix-dom.md (a própria regra de não-captura
//     de atributo de `<head>` da seção 4 é o precedente).
HeadScanResult dom_dump_scan_head(const std::string& source);

// EN: Full dump per docs/uix-dom.md: the `HEAD` record (derived from `source`, section 4) then
//     the `body` subtree in pre-order depth-first traversal (derived by walking `doc`, section
//     5), ending with a trailing newline (section 1). `source` MUST be the exact bytes that
//     were fed to RmlUi to produce `doc` -- a mismatched pair (e.g. `doc` loaded from one file,
//     `source` read from another) produces a HEAD record that does not describe `doc` at all,
//     silently, since this function has no way to detect that mismatch (it never re-parses
//     `source` itself, see `dom_dump_scan_head`'s own doc-comment for why).
//
//     PRECONDITION: `doc != nullptr`. A null document is a caller error (a failed `Load`/
//     `LoadDocumentFromMemory` should be checked and reported at the call site, same as every
//     other RmlUi-touching test in this repo already does), not a legitimate empty-body case --
//     docs/uix-dom.md section 3 states a document missing `<body>` is out of this format's
//     scope entirely, and `doc` being null is a strictly worse case than that. Asserted in
//     debug builds; UB if violated in release, same discipline as this file's own callers
//     already apply to `Bootstrap::context()` before dereferencing it.
// PT: Dump completo conforme docs/uix-dom.md: o registro `HEAD` (derivado de `source`, seção 4)
//     seguido da subárvore `body` em travessia pré-ordem profundidade-primeiro (derivada
//     percorrendo `doc`, seção 5), terminando com newline final (seção 1). `source` TEM que ser
//     os bytes exatos que foram alimentados ao RmlUi pra produzir `doc` -- um par desencontrado
//     (ex.: `doc` carregado de um arquivo, `source` lido de outro) produz um registro `HEAD`
//     que não descreve `doc` nenhum, silenciosamente, já que esta função não tem como detectar
//     esse desencontro (ela nunca reparseia `source` sozinha, ver o próprio doc-comment de
//     `dom_dump_scan_head` pro motivo).
//
//     PRECONDIÇÃO: `doc != nullptr`. Um documento nulo é erro do chamador (um `Load`/
//     `LoadDocumentFromMemory` que falhou deve ser checado e reportado no ponto de chamada,
//     mesma disciplina que todo outro teste deste repo que toca RmlUi já aplica), não um caso
//     legítimo de body-vazio -- a seção 3 do docs/uix-dom.md declara que um documento sem
//     `<body>` está inteiramente fora do escopo deste formato, e `doc` nulo é um caso
//     estritamente pior que esse. Verificado por assert em builds debug; UB se violado em
//     release, mesma disciplina que os próprios chamadores deste arquivo já aplicam a
//     `Bootstrap::context()` antes de desreferenciar.
std::string dom_dump_document(Rml::ElementDocument* doc, const std::string& source);

} // namespace glintfx
