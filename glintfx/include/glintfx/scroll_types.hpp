// SPDX-License-Identifier: Apache-2.0
// EN: Public alignment selector for the ScrollAlign overload of scroll_element_into_view()
//     (SCROLL-ALIGN, W26) -- a plain, engine-agnostic enum mirroring RmlUi's own
//     `Rml::ScrollAlignment` (Include/RmlUi/Core/ScrollTypes.h), which glintfx already wraps
//     one axis of but never exposed. No RmlUi type appears here, or anywhere else in glintfx's
//     public headers (the encapsulation invariant every header under glintfx/include/glintfx/
//     upholds; see AGENTS.md "gate de encapsulamento").
//
//     WHY THIS EXISTS: `scroll_element_into_view(id, bool align_with_top = true)` always
//     re-anchors the element to an edge (top or bottom), even when it is already fully
//     visible -- measured by a consumer with a controlled experiment (GusWorld, 2026-08-01):
//     an already-visible selected list item jumped `y=204 -> y=136` on every call. RmlUi 6.3
//     already ships the fix as `Rml::ScrollAlignment::Adaptive` ("do not scroll if already in
//     view, otherwise align to the center") -- this header is the missing plumbing to reach it.
//
//     VERTICAL AXIS ONLY (deliberate scope cut, SCROLL-ALIGN): RmlUi's own
//     `ScrollIntoViewOptions` struct additionally exposes independent horizontal alignment,
//     `ScrollBehavior` (Instant/Smooth animation), and `ScrollParentage` (All/Closest ancestor
//     chain) -- none of those three axes are reachable through this enum or the overload it
//     feeds. A future extension point if a consumer needs them; RmlUi's own defaults apply in
//     the meantime (Nearest horizontal, Instant behavior, All parentage -- the exact same
//     defaults the pre-existing `bool` overload already uses under the hood, see
//     bootstrap.cpp).
//
//     RELATIONSHIP TO THE bool OVERLOAD: `scroll_element_into_view(id, bool align_with_top)`
//     is NOT deprecated by this addition and stays exactly as-is -- there is a real consumer on
//     it, and deprecating now would be pure warning churn in their build for no gain. `Start`/
//     `End` below reproduce `align_with_top=true`/`false` byte-for-byte (same
//     Rml::ScrollAlignment values under the hood); prefer the enum overload in new code for the
//     `Adaptive` behaviour, but either overload remains fully supported.
//
// PT: Seletor público de alinhamento para a sobrecarga ScrollAlign de scroll_element_into_view()
//     (SCROLL-ALIGN, W26) -- um enum simples, agnóstico de engine, espelhando o próprio
//     `Rml::ScrollAlignment` do RmlUi (Include/RmlUi/Core/ScrollTypes.h), do qual a glintfx já
//     encapsulava um eixo mas nunca expunha. Nenhum tipo RmlUi aparece aqui, nem em nenhum
//     outro header público da glintfx (o invariante de encapsulamento que todo header sob
//     glintfx/include/glintfx/ sustenta; ver AGENTS.md "gate de encapsulamento").
//
//     POR QUE ISTO EXISTE: `scroll_element_into_view(id, bool align_with_top = true)` sempre
//     reancora o elemento a uma borda (topo ou fundo), mesmo quando ele já está totalmente
//     visível -- medido por um consumidor com experimento controlado (GusWorld, 2026-08-01):
//     um item de lista já selecionado e visível pulava `y=204 -> y=136` a cada chamada. O
//     RmlUi 6.3 já embarca o conserto como `Rml::ScrollAlignment::Adaptive` ("não rola se já
//     está visível, senão alinha ao centro") -- este header é a fiação que faltava pra alcançá-lo.
//
//     SÓ EIXO VERTICAL (corte de escopo deliberado, SCROLL-ALIGN): o próprio struct
//     `ScrollIntoViewOptions` do RmlUi expõe adicionalmente alinhamento horizontal
//     independente, `ScrollBehavior` (animação Instant/Smooth), e `ScrollParentage` (cadeia de
//     ancestrais All/Closest) -- nenhum desses três eixos é alcançável por este enum ou pela
//     sobrecarga que ele alimenta. Um ponto de extensão futuro se algum consumidor precisar;
//     os defaults do próprio RmlUi valem enquanto isso (Nearest horizontal, Instant behavior,
//     All parentage -- exatamente os mesmos defaults que a sobrecarga `bool` pré-existente já
//     usa por baixo dos panos, ver bootstrap.cpp).
//
//     RELAÇÃO COM A SOBRECARGA bool: `scroll_element_into_view(id, bool align_with_top)` NÃO é
//     depreciada por esta adição e continua exatamente como está -- há consumidor real usando
//     ela, e depreciar agora seria puro churn de warning no build dele sem ganho nenhum.
//     `Start`/`End` abaixo reproduzem `align_with_top=true`/`false` byte a byte (os mesmos
//     valores de Rml::ScrollAlignment por baixo dos panos); prefira a sobrecarga de enum em
//     código novo para o comportamento `Adaptive`, mas as duas sobrecargas seguem plenamente
//     suportadas.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <cstdint>

namespace glintfx {

// EN: Vertical scroll alignment for scroll_element_into_view(id, ScrollAlign). Mirrors
//     Rml::ScrollAlignment 1:1 (same five values, same order) -- see this header's top comment
//     for the full rationale and for what is deliberately NOT exposed (horizontal axis,
//     animated behavior, parentage).
//
//     Start:    align the element to the top of the scrollable view (== align_with_top=true).
//     Center:   align the element to the center of the scrollable view.
//     End:      align the element to the bottom of the scrollable view (== align_with_top=false).
//     Nearest:  scroll the minimal amount needed to bring the element fully into view; a no-op
//               if it is already fully visible.
//     Adaptive: like Nearest's no-op-if-visible check, but re-centers (rather than
//               minimal-move-aligns) when a scroll IS needed -- THIS is the value that fixes
//               the re-anchor-on-every-call bug described in the top comment.
//
// PT: Alinhamento vertical de rolagem para scroll_element_into_view(id, ScrollAlign). Espelha
//     Rml::ScrollAlignment 1:1 (os mesmos cinco valores, na mesma ordem) -- ver o comentário de
//     topo deste header pro racional completo e pro que é deliberadamente NÃO exposto (eixo
//     horizontal, behavior animado, parentage).
//
//     Start:    alinha o elemento ao topo da área rolável (== align_with_top=true).
//     Center:   alinha o elemento ao centro da área rolável.
//     End:      alinha o elemento ao fundo da área rolável (== align_with_top=false).
//     Nearest:  rola o mínimo necessário para trazer o elemento totalmente para a área visível;
//               é um no-op se ele já está totalmente visível.
//     Adaptive: como a checagem de no-op-se-visível do Nearest, mas recentraliza (em vez de
//               alinhar por movimento mínimo) quando uma rolagem É necessária -- ESTE é o valor
//               que conserta o bug de reancorar a cada chamada descrito no comentário de topo.
enum class ScrollAlign : std::uint8_t {
  Start,
  Center,
  End,
  Nearest,
  Adaptive,
};

} // namespace glintfx
