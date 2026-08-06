// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-RCSS-DUMP-A` -- dumper over the REAL `Style::ComputedValues`/`Element::GetProperty`
//     of the RmlUi engine, emitting the format defined in `docs/uix-rcss.md` (the SOLE contract
//     this file implements; do not add any behaviour not written there without editing that
//     document first). This is side **A** of the `RMLX-2` differential oracle -- side **B**
//     walks glintfx's own RCSS engine (`glintfx/src/uix/style/`) instead. By design this file's
//     author has NOT read side B's source, and side B's author must not read this one either
//     (see `docs/uix-rcss.md`'s own "Why this document exists" section for why a shared
//     author/reader would poison the differential oracle). Mirrors the exact same discipline
//     `dom_dump.hpp` (the `RMLX-1`/side-A sibling) already established for this repo.
//
//     Confinement: this header/its .cpp live in `glintfx/src/rml/` -- the ONLY place in this
//     repo permitted to `#include <RmlUi/...>` (`tools/check_rml_whitelist.sh`). Same
//     pImpl-adjacent forward-declaration discipline as `dom_dump.hpp`: the public surface below
//     only forward-declares `Rml::ElementDocument`, so a caller OUTSIDE `src/rml/` could
//     technically link against `rcss_dump_document()` without itself needing an `Rml::` token --
//     in practice this wave's only callers are this file's own test executables, which ALSO
//     live inside `src/rml/` (see `rcss_dump_corpus_sanity.cpp` / `rcss_dump_worked_examples.cpp`)
//     precisely so a NEW test never has to fight check (b)/(c) of the whitelist gate.
//
//     Two free functions with ZERO `Rml::` surface (`rcss_quantize`, `rcss_color_hex`) are
//     exposed too, on purpose: `docs/uix-rcss.md` section 8's quantization algorithm and section
//     7.1's color canonicalization are pure functions, testable in total isolation from any
//     RmlUi document/element -- exactly what section 15.4's worked example (the quantization
//     boundary, exact tie + one step outside, both signs) needs, and exactly the same
//     "test the pure function directly, not just through property serialization" discipline the
//     brief's own house rule asks for.
// PT: `UIX-RCSS-DUMP-A` -- dumper sobre o `Style::ComputedValues`/`Element::GetProperty` REAIS
//     do motor RmlUi, emitindo o formato definido em `docs/uix-rcss.md` (o ÚNICO contrato que
//     este arquivo implementa; não somar comportamento nenhum que não esteja escrito lá sem
//     editar aquele documento primeiro). É o lado **A** do oráculo diferencial da `RMLX-2` -- o
//     lado **B** percorre o motor RCSS próprio da glintfx (`glintfx/src/uix/style/`) em vez
//     deste. De propósito o autor deste arquivo NÃO leu o fonte do lado B, e o autor do lado B
//     também não pode ler este (ver a própria seção "Por que este documento existe" do
//     docs/uix-rcss.md pro motivo de um autor/leitor compartilhado envenenar o oráculo
//     diferencial). Espelha a mesma disciplina que o `dom_dump.hpp` (o irmão `RMLX-1`/lado-A)
//     já estabeleceu neste repo.
//
//     Confinamento: este header/seu .cpp moram em `glintfx/src/rml/` -- o ÚNICO lugar deste
//     repo com permissão de `#include <RmlUi/...>` (`tools/check_rml_whitelist.sh`). Mesma
//     disciplina pImpl-adjacente de forward-declaration do `dom_dump.hpp`: a superfície pública
//     abaixo só forward-declara `Rml::ElementDocument`, então um chamador FORA de `src/rml/`
//     poderia tecnicamente linkar contra `rcss_dump_document()` sem precisar de nenhum token
//     `Rml::` -- na prática os únicos chamadores desta onda são os próprios executáveis de teste
//     deste arquivo, que TAMBÉM moram dentro de `src/rml/` (ver `rcss_dump_corpus_sanity.cpp` /
//     `rcss_dump_worked_examples.cpp`) precisamente pra um teste NOVO nunca precisar brigar com
//     o check (b)/(c) do gate de whitelist.
//
//     Duas funções livres com ZERO superfície `Rml::` (`rcss_quantize`, `rcss_color_hex`) também
//     são expostas, de propósito: o algoritmo de quantização da seção 8 e a canonicalização de
//     cor da seção 7.1 do docs/uix-rcss.md são funções puras, testáveis em isolamento total de
//     qualquer documento/elemento RmlUi -- exatamente o que o exemplo trabalhado da seção 15.4
//     (a fronteira de quantização, empate exato + um passo pra fora, nos dois sinais) precisa, e
//     exatamente a mesma disciplina "teste a função pura direto, não só através da serialização
//     de propriedade" que a própria regra da casa do brief pede.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstdint>
#include <string>

namespace Rml {
class ElementDocument;
} // namespace Rml

namespace glintfx {

// EN: docs/uix-rcss.md section 8's quantization algorithm, verbatim: widen to double, scale by
//     10^4, round-half-AWAY-FROM-ZERO (explicit, not "whatever printf does" -- see that section's
//     own justification), canonicalize -0 to 0, print fixed-point with exactly 4 digits after
//     '.', no exponent, '-' prefix iff negative, '.' always the decimal point (never locale).
//     Implemented on INTEGER cents-of-1e-4 arithmetic after the round step (not by formatting the
//     rounded double directly) so the printed digits are exact -- no double-to-decimal-string
//     rounding surprise can reintroduce the very ambiguity this function exists to remove.
// PT: O algoritmo de quantização da seção 8 do docs/uix-rcss.md, ao pé da letra: larga pra
//     double, escala por 10^4, arredonda meio-PRA-LONGE-DE-ZERO (explícito, não "o que o printf
//     fizer" -- ver a própria justificativa daquela seção), canonicaliza -0 pra 0, imprime
//     ponto-fixo com exatamente 4 dígitos depois do '.', sem expoente, prefixo '-' sse negativo,
//     '.' sempre o separador decimal (nunca locale). Implementado em aritmética inteira de
//     centésimos-de-1e-4 depois do passo de arredondamento (não formatando o double arredondado
//     direto) pra os dígitos impressos serem exatos -- nenhuma surpresa de arredondamento
//     double-pra-string-decimal pode reintroduzir a própria ambiguidade que esta função existe
//     pra remover.
std::string rcss_quantize(float x);

// EN: docs/uix-rcss.md section 7.1's canonical color form: 8-digit lowercase hex `#rrggbbaa`,
//     straight (non-premultiplied) alpha -- the CALLER is responsible for having already
//     un-premultiplied a `ColourbPremultiplied`-typed field (box-shadow layers, gradient stops)
//     before calling this; this function only formats 4 already-straight bytes.
// PT: A forma canônica de cor da seção 7.1 do docs/uix-rcss.md: hex 8 dígitos minúsculo
//     `#rrggbbaa`, alfa reto (não-premultiplicado) -- o CHAMADOR é responsável por já ter
//     des-premultiplicado um campo tipado `ColourbPremultiplied` (camadas de box-shadow, stops
//     de gradiente) antes de chamar isto; esta função só formata 4 bytes já retos.
std::string rcss_color_hex(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);

// EN: Full dump per docs/uix-rcss.md: two `STATE` blocks (`none`, `hover-all`, section 4), each
//     a complete, independent, full-tree `PROPS`/`PROP` enumeration (section 3) over the same
//     `body`-rooted, pre-order depth-first tree docs/uix-dom.md section 5 defines (reused
//     unchanged, section 2). `hover-all` forces `:hover` true on EVERY element in `doc` before
//     its own traversal (`Element::SetPseudoClass` + a `Context::Update()` to flush the cascade),
//     and un-forces it again afterward so a caller dumping multiple documents in the same
//     process does not leak forced-hover state across calls.
//
//     PRECONDITION: `doc != nullptr` (same discipline as `dom_dump_document()`'s own
//     precondition -- a failed `Load`/`LoadDocumentFromMemory` is a caller error, checked at the
//     call site). Asserted in debug builds; UB if violated in release.
// PT: Dump completo conforme docs/uix-rcss.md: dois blocos `STATE` (`none`, `hover-all`, seção
//     4), cada um uma enumeração `PROPS`/`PROP` completa e independente da árvore inteira (seção
//     3) sobre a MESMA árvore raiz-`body`, pré-ordem profundidade-primeiro que a seção 5 do
//     docs/uix-dom.md define (reusada sem mudança, seção 2). `hover-all` força `:hover`
//     verdadeiro em TODO elemento de `doc` antes da própria travessia (`Element::SetPseudoClass`
//     + um `Context::Update()` pra drenar a cascata), e desfaz a força depois -- um chamador que
//     dumpa vários documentos no mesmo processo não vaza estado de hover forçado entre chamadas.
//
//     PRECONDIÇÃO: `doc != nullptr` (mesma disciplina da própria precondição do
//     `dom_dump_document()` -- um `Load`/`LoadDocumentFromMemory` que falhou é erro do chamador,
//     checado no ponto de chamada). Verificado por assert em builds debug; UB se violado em
//     release.
std::string rcss_dump_document(Rml::ElementDocument* doc);

} // namespace glintfx
