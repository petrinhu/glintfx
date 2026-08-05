// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6b -- the dumper that walks glintfx's OWN DOM (`dom_tree.hpp`'s `Document`/
//     `Element`/`Text`) and serializes it into the canonical, byte-exact format
//     `docs/uix-dom.md` ("uix-dom.md" hereafter) defines. Public surface is deliberately ONE
//     function, `dump_document` -- mirrors `parser.hpp`'s own "one function, no exposed
//     implementation-detail class" shape.
//
//     THIS MODULE IS ONE HALF OF A DIFFERENTIAL ORACLE (uix-dom.md's own "Why this document
//     exists" section) -- its sibling, `S6a`, walks the REAL `Rml::ElementDocument` tree,
//     confined to `glintfx/src/rml/`, and is written by a DIFFERENT agent who never reads this
//     file. This file was written reading ONLY `docs/uix-dom.md` -- never `S6a`'s source, which
//     does not exist in this agent's context at all. `S7` (not this slice) diffs the two dumpers'
//     output over the shared corpus; a divergence there is either uix-dom.md section 9's class
//     (a) (a bug in one of the two dumpers), (b) (an upstream RmlUi normalization neither dumper
//     replicates yet), or (c) (an out-of-subset construct) -- never something this file's own
//     implementer decides unilaterally.
//
//     WHY THIS LIVES IN `src/uix/dom/`, NOT `tests/uix/`: `dump_document` is not a test -- it
//     produces one HALF of the oracle input `S7` (a future, separate slice) will consume, and a
//     future consumer outside this arc (a debugging tool, a snapshot-test harness for `RMLX-2+`)
//     could plausibly want to call it directly. It has no `assert`/`check` inside it and no
//     `main()` -- exactly the same "production code lives in src/, the tests that exercise it
//     live in tests/" split every other S1-S4 module in this directory already follows. It is
//     added to THIS directory's existing `glintfx_uix_dom` STATIC target (one more source file in
//     `CMakeLists.txt`'s single `add_library(...)` line, per that file's own single-line-edit
//     warning) -- same "own CMake, zero RmlUi/GLFW/GL" build-cost discipline as `lexer.*`/
//     `dom_tree.*`/`parser.*`/`dom_api.*`, because a dumper that walks THIS tree has, and needs,
//     no more link surface than the tree itself.
//
//     DETERMINISM IS THE WHOLE POINT (uix-dom.md's own framing: the two dumpers may run on
//     different machines) -- this function must never read anything that varies across runs or
//     machines: no pointer address, no `std::unordered_map`/`std::unordered_set` iteration order,
//     no wall-clock time, no environment variable, no locale-aware comparison/formatting. Every
//     ordering `dump_document` relies on is either a `dom_tree.hpp`-level invariant already
//     guaranteed at STORAGE time (`Element::classes()` is `std::set<std::string, std::less<>>`,
//     `Element::attributes()` is `std::map<std::string, std::string, std::less<>>` -- both
//     byte-wise ascending, deduplicated, per `dom_tree.hpp`'s own header comment) or this file's
//     own linear, single-pass byte scan (`escape`, below) -- nothing here iterates a hash-ordered
//     container or depends on where in memory a `Node` happens to live.
// PT: RMLX-1/S6b -- o dumper que percorre o DOM PRÓPRIO da glintfx (o `Document`/`Element`/`Text`
//     do `dom_tree.hpp`) e o serializa no formato canônico, byte-exato, que o `docs/uix-dom.md`
//     ("uix-dom.md" daqui em diante) define. Superfície pública deliberadamente UMA função só,
//     `dump_document` -- espelha a própria forma "uma função só, sem classe de implementação
//     exposta" do `parser.hpp`.
//
//     ESTE MÓDULO É UMA METADE DE UM ORÁCULO DIFERENCIAL (a própria seção "Por que este documento
//     existe" do uix-dom.md) -- o irmão dele, `S6a`, percorre a árvore `Rml::ElementDocument`
//     REAL, confinado a `glintfx/src/rml/`, e é escrito por um agente DIFERENTE que nunca lê este
//     arquivo. Este arquivo foi escrito lendo SÓ o `docs/uix-dom.md` -- nunca o fonte da `S6a`,
//     que não existe no contexto deste agente de jeito nenhum. A `S7` (não esta fatia) faz o diff
//     da saída dos dois dumpers sobre o corpus compartilhado; uma divergência ali é ou a classe
//     (a) da seção 9 do uix-dom.md (bug num dos dois dumpers), (b) (uma normalização do RmlUi
//     upstream que nenhum dos dois dumpers replica ainda), ou (c) (uma construção fora do
//     subconjunto) -- nunca algo que o implementer deste arquivo decide unilateralmente.
//
//     POR QUE ISTO MORA EM `src/uix/dom/`, NÃO EM `tests/uix/`: `dump_document` não é um teste --
//     produz UMA METADE do input do oráculo que a `S7` (uma futura fatia separada) vai consumir, e
//     um futuro consumidor fora deste arco (uma ferramenta de debug, um harness de snapshot-test
//     pra `RMLX-2+`) poderia plausivelmente querer chamar isto direto. Não tem `assert`/`check`
//     nenhum dentro nem `main()` nenhum -- exatamente a mesma divisão "código de produção mora em
//     src/, os testes que o exercitam moram em tests/" que todo outro módulo S1-S4 deste
//     diretório já segue. É somado ao alvo STATIC `glintfx_uix_dom` já existente deste diretório
//     (mais um arquivo-fonte na linha única `add_library(...)` do `CMakeLists.txt`, pelo próprio
//     aviso de edição-de-linha-única daquele arquivo) -- mesma disciplina de custo-de-build "CMake
//     próprio, zero RmlUi/GLFW/GL" que `lexer.*`/`dom_tree.*`/`parser.*`/`dom_api.*`, porque um
//     dumper que percorre ESTA árvore não tem, nem precisa de, mais superfície de link do que a
//     própria árvore.
//
//     DETERMINISMO É O PONTO INTEIRO (a própria formulação do uix-dom.md: os dois dumpers podem
//     rodar em máquinas diferentes) -- esta função nunca pode ler nada que varie entre execuções
//     ou máquinas: nenhum endereço de ponteiro, nenhuma ordem de iteração de
//     `std::unordered_map`/`std::unordered_set`, nenhum relógio de parede, nenhuma variável de
//     ambiente, nenhuma comparação/formatação sensível a locale. Toda ordenação de que
//     `dump_document` depende é ou um invariante de NÍVEL `dom_tree.hpp` já garantido em tempo de
//     ARMAZENAMENTO (`Element::classes()` é `std::set<std::string, std::less<>>`,
//     `Element::attributes()` é `std::map<std::string, std::string, std::less<>>` -- os dois
//     ascendentes byte-a-byte, deduplicados, pelo próprio comentário de cabeçalho do
//     `dom_tree.hpp`) ou o próprio scan de byte linear, passe-único, deste arquivo (`escape`,
//     abaixo) -- nada aqui itera um container ordenado-por-hash nem depende de onde na memória um
//     `Node` por acaso mora.
//
//     WHAT THIS SLICE DELIBERATELY DOES NOT RE-DO (already a tree-level invariant, per
//     `dom_tree.hpp`'s own header comment point (1)): the whitespace-only-text EXISTENCE filter
//     (uix-dom.md section 6a) is NOT applied here. `Element::append_child` already guarantees no
//     `Element`'s `children()` ever contains a whitespace-only `Text` -- so `Element::child_count()`
//     IS ALREADY the "surviving children" count uix-dom.md section 3 requires for `CHILDREN <n>`
//     and for dense, gap-free child indices; this dumper's own traversal loop needs no filtering
//     logic of its own, it just enumerates `children()` in order. Re-filtering here would not be
//     "extra safety", it would be a SECOND, independently-written copy of the exact whitespace
//     predicate `dom_tree.cpp` already applies -- precisely the "private assumption invented
//     independently" failure mode uix-dom.md's own header warns about, one layer further in.
// PT: O QUE ESTA FATIA DELIBERADAMENTE NÃO REFAZ (já é invariante de nível-árvore, pelo próprio
//     ponto (1) do comentário de cabeçalho do `dom_tree.hpp`): o filtro de EXISTÊNCIA de texto
//     só-whitespace (seção 6a do uix-dom.md) NÃO é aplicado aqui. O `Element::append_child` já
//     garante que os `children()` de nenhum `Element` contêm um `Text` só-whitespace -- então
//     `Element::child_count()` JÁ É a contagem de "filhos sobreviventes" que a seção 3 do
//     uix-dom.md exige pro `CHILDREN <n>` e pra índices de filho densos, sem buraco; o próprio
//     laço de travessia deste dumper não precisa de lógica de filtro nenhuma própria, só enumera
//     `children()` em ordem. Re-filtrar aqui não seria "segurança extra", seria uma SEGUNDA cópia,
//     escrita de forma independente, do mesmo predicado de whitespace exato que o `dom_tree.cpp`
//     já aplica -- precisamente o modo de falha "suposição privada inventada de forma
//     independente" que o próprio cabeçalho do uix-dom.md avisa, uma camada mais pra dentro.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <string>
#include <string_view>

#include "uix/dom/dom_tree.hpp"

namespace glintfx::uix {

// EN: Applies uix-dom.md section 2's escaping table (backslash, then '\n'/'\r'/'\t') to `s` in a
//     single left-to-right pass over the SOURCE bytes -- each source byte is visited exactly
//     once and mapped to either itself or its two-character escape sequence, so the "order"
//     section 2 specifies (backslash before the other three) is satisfied by construction: a
//     one-pass byte-to-output mapping can never re-scan a backslash IT ITSELF just emitted (the
//     failure mode a sequential find-and-replace over the growing output string would risk).
//     Every byte outside the 4-character escape set -- including the literal ASCII space and
//     every byte of a multi-byte UTF-8 sequence -- passes through unchanged, per section 2's own
//     text. Exposed (not `dumper.cpp`-local) because it is exactly the primitive uix-dom.md
//     section 2 names for "every value-bearing field", so a future caller that needs to escape a
//     single field on its own (a diagnostic tool, a hand test) does not have to reimplement it.
// PT: Aplica a tabela de escape da seção 2 do uix-dom.md (barra invertida, depois '\n'/'\r'/'\t')
//     a `s` num único passe esquerda-pra-direita sobre os bytes-FONTE -- cada byte-fonte é
//     visitado exatamente uma vez e mapeado ou pra ele mesmo ou pra sua sequência de escape de
//     dois caracteres, então a "ordem" que a seção 2 especifica (barra invertida antes das outras
//     três) é satisfeita por construção: um mapeamento byte-pra-saída de passe único nunca pode
//     re-escanear uma barra invertida que ELE MESMO acabou de emitir (o modo de falha que um
//     find-and-replace sequencial sobre a string de saída crescente arriscaria). Todo byte fora
//     do conjunto de escape de 4 caracteres -- incluindo o espaço ASCII literal e todo byte de uma
//     sequência UTF-8 multi-byte -- passa sem mudança, pelo próprio texto da seção 2. Exposta (não
//     local ao `dumper.cpp`) porque é exatamente a primitiva que a seção 2 do uix-dom.md nomeia
//     pra "todo campo que carrega valor", então um futuro chamador que precise escapar um único
//     campo sozinho (uma ferramenta de diagnóstico, um teste à mão) não precisa reimplementá-la.
std::string escape(std::string_view s);

// EN: Serializes `document` into uix-dom.md's canonical dump format: the `HEAD` record (section
//     4) followed by the `body` subtree in pre-order depth-first traversal (section 5), one fact
//     per line, every line ending in `\n` including the last (section 1: "the file always ends
//     with a trailing newline"). Never throws, never fails -- there is no invalid `Document`
//     this function cannot dump (a `Document` is always well-formed by `dom_tree.hpp`'s own
//     construction invariants; see this file's header comment for exactly which of those
//     invariants this function leans on instead of re-checking).
// PT: Serializa `document` no formato de dump canônico do uix-dom.md: o registro `HEAD` (seção 4)
//     seguido da subárvore `body` em travessia pré-ordem, profundidade primeiro (seção 5), um
//     fato por linha, toda linha terminando em `\n` inclusive a última (seção 1: "o arquivo
//     sempre termina com newline final"). Nunca lança exceção, nunca falha -- não existe
//     `Document` inválido que esta função não consiga dumpar (um `Document` é sempre bem-formado
//     pelos próprios invariantes de construção do `dom_tree.hpp`; ver o comentário de cabeçalho
//     deste arquivo pra exatamente quais desses invariantes esta função se apoia em vez de
//     re-checar).
std::string dump_document(const Document& document);

} // namespace glintfx::uix
