// SPDX-License-Identifier: Apache-2.0
// EN: byte_ceiling_dedup_sanity (TETO-DEDUP, W26, 2026-08-04) -- proves the "hard ceiling on any
//     untrusted byte stream" policy (src/byte_ceiling.hpp, `kMaxUntrustedFileBytes`) is a SHARED
//     VALUE the source sites actually reference, not five independently-typed literals that
//     happen to read the same today (the exact ambiguity TETO-CALIBRE's own postmortem raised:
//     "reuse of the VALUE, not the SYMBOL" -- see byte_ceiling.hpp's own top comment for the full
//     genesis). This test is a compile-time proof, not a runtime one, and that split matters:
//       - `glintfx::kMaxImageDecodeBytes` (image_decode.hpp, namespace-scope,
//         externally visible) and `glintfx::BaseUrlFileInterface::kMaxFileBytes`
//         (base_url_file_interface.hpp, public static constexpr class member, externally visible)
//         are checked HERE, directly, via `static_assert` against `glintfx::kMaxUntrustedFileBytes`
//         -- both symbols are reachable from outside their own translation unit, so an external
//         test can name them.
//       - `frame_capture.cpp`'s `kMaxCaptureBytes` (anonymous namespace, private to that .cpp) and
//         `render_gl3.cpp`'s `Gl3RenderInterface::kMaxAssetFileBytes` (private class member of a
//         class defined entirely inside that .cpp) are NOT reachable from any other translation
//         unit by construction -- narrowing their visibility on purpose is exactly what keeps them
//         each their own independent checkpoint (see byte_ceiling.hpp's own "what dedup does and
//         does not change" paragraph). Those two are instead proven in-place: each site carries
//         its own `static_assert(kMaxXxxBytes == glintfx::kMaxUntrustedFileBytes, ...)` right next
//         to its declaration, in the SAME translation unit -- a compile-time invariant this test
//         cannot reach from outside, so it is asserted where visibility allows it, not skipped.
//     Any one of the four sites drifting from `kMaxUntrustedFileBytes` (a stray re-typed literal,
//     a forgotten update after a future policy change) is a BUILD FAILURE somewhere in the tree,
//     not a silent pass -- exactly the property "the same 256 MiB as five independent literals"
//     did not have before this fatia.
//     Deliberately does NOT touch `gamepad.cpp`'s `kMaxMappingsFileBytes`: TETO-CALIBRE (W25)
//     right-sized it to 1 MiB, diverging from this default ON PURPOSE (derived from the real
//     vendored gamecontrollerdb_linux.txt artifact, not from this policy) -- asserting it AGAINST
//     `kMaxUntrustedFileBytes` here would be asserting the wrong invariant (that it should NOT
//     have diverged), the opposite of what TETO-CALIBRE decided.
//     No RmlUi document, no GL context, no window -- `main()` only prints PASS, because every
//     check this file performs is a `static_assert` (rejected at compile time, never reaches
//     runtime); a `main()` returning 0 is still needed so this links into an executable ctest can
//     run and report on, same convention as this suite's other pure unit tests.
// PT: byte_ceiling_dedup_sanity (TETO-DEDUP, W26, 2026-08-04) -- prova que a política "teto rígido
//     sobre qualquer stream de bytes não confiável" (src/byte_ceiling.hpp, `kMaxUntrustedFileBytes`)
//     é um VALOR COMPARTILHADO que os pontos de chamada de fato referenciam, não cinco literais
//     tipados independentemente que hoje coincidem em ler o mesmo número (a ambiguidade exata que
//     o próprio post-mortem do TETO-CALIBRE levantou: "reuso do VALOR, não do SÍMBOLO" -- ver o
//     próprio comentário de topo de byte_ceiling.hpp pra gênese completa). Este teste é uma prova
//     em tempo de COMPILAÇÃO, não em runtime, e essa divisão importa:
//       - `glintfx::kMaxImageDecodeBytes` (image_decode.hpp, escopo de namespace, visível
//         externamente) e `glintfx::BaseUrlFileInterface::kMaxFileBytes`
//         (base_url_file_interface.hpp, membro `static constexpr` público de classe, visível
//         externamente) são checados AQUI, direto, via `static_assert` contra
//         `glintfx::kMaxUntrustedFileBytes` -- os dois símbolos são alcançáveis de fora da própria
//         unidade de tradução, então um teste externo consegue nomeá-los.
//       - `kMaxCaptureBytes` de `frame_capture.cpp` (namespace anônimo, privado àquele .cpp) e
//         `Gl3RenderInterface::kMaxAssetFileBytes` de `render_gl3.cpp` (membro privado de classe
//         definida inteiramente dentro daquele .cpp) NÃO são alcançáveis de nenhuma outra unidade
//         de tradução por construção -- estreitar a visibilidade deles de propósito é exatamente o
//         que mantém cada um como o próprio checkpoint independente (ver o próprio parágrafo "o que
//         o dedup muda e o que não muda" de byte_ceiling.hpp). Esses dois são provados no próprio
//         lugar em vez disso: cada ponto carrega o próprio
//         `static_assert(kMaxXxxBytes == glintfx::kMaxUntrustedFileBytes, ...)` bem ao lado da
//         própria declaração, na MESMA unidade de tradução -- um invariante em tempo de compilação
//         que este teste não alcança de fora, então é afirmado onde a visibilidade permite, não
//         pulado.
//     Qualquer um dos quatro pontos divergindo de `kMaxUntrustedFileBytes` (um literal re-tipado
//     por engano, uma atualização esquecida após uma futura mudança de política) é uma FALHA DE
//     BUILD em algum lugar da árvore, não uma passagem silenciosa -- exatamente a propriedade que
//     "os mesmos 256 MiB como cinco literais independentes" não tinha antes desta fatia.
//     Deliberadamente NÃO toca `kMaxMappingsFileBytes` de `gamepad.cpp`: o TETO-CALIBRE (W25)
//     recalibrou pra 1 MiB, divergindo deste default DE PROPÓSITO (derivado do artefato real
//     vendorizado gamecontrollerdb_linux.txt, não desta política) -- afirmar contra
//     `kMaxUntrustedFileBytes` aqui estaria afirmando o invariante errado (que ele NÃO deveria ter
//     divergido), o oposto do que o TETO-CALIBRE decidiu.
//     Sem documento RmlUi, sem contexto GL, sem janela -- `main()` só imprime PASS, porque toda
//     checagem que este arquivo faz é um `static_assert` (rejeitado em tempo de compilação, nunca
//     chega a runtime); um `main()` retornando 0 ainda é necessário pra que isto linke num
//     executável que o ctest consiga rodar e reportar, mesma convenção das outras unit tests puras
//     desta suíte.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/byte_ceiling.hpp"
#include "../src/image_decode.hpp"
#include "../src/rml/base_url_file_interface.hpp"

#include <cstdio>

namespace {

static_assert(glintfx::kMaxUntrustedFileBytes == 256u * 1024u * 1024u,
              "byte_ceiling.hpp's own kMaxUntrustedFileBytes must stay 256 MiB unless the whole "
              "default policy is deliberately re-derived (TETO-DEDUP's own point: this is the ONE "
              "place that edit now happens)");

static_assert(glintfx::kMaxImageDecodeBytes == glintfx::kMaxUntrustedFileBytes,
              "image_decode.hpp's kMaxImageDecodeBytes drifted from the shared "
              "glintfx::kMaxUntrustedFileBytes policy constant (TETO-DEDUP)");

static_assert(static_cast<std::size_t>(glintfx::BaseUrlFileInterface::kMaxFileBytes) ==
                  glintfx::kMaxUntrustedFileBytes,
              "base_url_file_interface.hpp's kMaxFileBytes drifted from the shared "
              "glintfx::kMaxUntrustedFileBytes policy constant (TETO-DEDUP)");

} // namespace

int main() {
  std::puts("byte_ceiling_dedup_sanity: PASS (all checks are static_assert, compile-time only)");
  return 0;
}
