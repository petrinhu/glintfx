// SPDX-License-Identifier: Apache-2.0
// EN: Verifies the SCROLL-ALIGN feature (W26, consumer-driven by GusWorld: a controlled
//     experiment measured scroll_element_into_view() re-anchoring an ALREADY VISIBLE element,
//     y=204 -> y=136, an unwanted jump) through glintfx::UiLayer's PUBLIC API only -- the new
//     `scroll_element_into_view(const char* id, ScrollAlign align)` overload
//     (glintfx/include/glintfx/scroll_types.hpp), never Bootstrap/Engine/Rml::Element directly.
//
//     Reuses the shared scroll_scene.rml/.rcss fixture from scroll_sanity.cpp (see that file's
//     header for the full scene oracle: #scroller is a 120x100 border-box viewport over 30 x
//     30px items, [0,800] scrollable range). This file adds ONE more oracle on top of it:
//     #item-10's content-local box is [300, 330] -- exactly the item scroll_sanity.cpp's own
//     mutation-testing note (B4) already established gives two DISCRIMINABLE, non-saturated
//     results for align_with_top=true/false (~300 / ~230), which is exactly why it is reused
//     here instead of picking a fresh item.
//
//     (1) THE BUG ITSELF, reproduced as an assertion: with the viewport scrolled to 250
//         (view=[250,350]), #item-10 [300,330] is ALREADY fully inside it -- begin_offset=50>=0
//         and end_offset=-20<=0 in RmlUi's own GetScrollOffsetDelta (Element.cpp) terms, the
//         exact condition both Nearest and Adaptive treat as "already visible, don't scroll".
//         scroll_element_into_view(id, ScrollAlign::Adaptive) must leave scroll_top UNCHANGED --
//         this is the delta-zero assertion that stands in for the consumer's y=204 case (their
//         measured y=204 -> y=136 IS a non-zero delta on an already-visible element; this test
//         pins the delta at exactly zero, the fixed behaviour).
//     (2) THE FIX ACTUALLY SCROLLS when needed: with the viewport at 0 (view=[0,100]),
//         #item-10 [300,330] is fully OUT of view -- Adaptive must scroll, landing on the
//         midpoint (300+230)/2 = 265 per GetScrollOffsetDelta's Adaptive branch (average of the
//         Start/End oracles below, itself a derived-not-guessed number).
//     (3) ScrollAlign::Start reproduces the bool overload's align_with_top=true byte-for-byte
//         (same Rml::ScrollAlignment::Start mapping under the hood) -- proven by DIRECT
//         A/B comparison against the pre-existing bool overload, not by hardcoding the same
//         magic number twice. Likewise ScrollAlign::End reproduces align_with_top=false.
//     (4) THE ANTI-AMBIGUITY PROOF: a single-argument call, scroll_element_into_view(id), must
//         still compile and dispatch to the OLD bool overload (align_with_top defaults to
//         true) -- the enum overload below is deliberately declared WITHOUT a default argument
//         so this stays unambiguous. The proof is really the fact that this translation unit
//         builds at all (an ambiguous overload set is a hard compile error, not a runtime
//         failure); the assertion below additionally pins the OLD overload's own oracle
//         (item-29 saturates to the clamped 800, per scroll_sanity.cpp's B3) as a regression
//         guard on top of the compile-time proof.
//     (5) Hardening parity with the bool overload (AUD-TEC-5 convention): null id / empty id
//         "" / unknown id / no-document-loaded-yet on the NEW overload all return false without
//         crashing, same guard shape as every other scroll method in this codebase.
//
// PT: Verifica a feature SCROLL-ALIGN (W26, consumer-driven pelo GusWorld: um experimento
//     controlado mediu scroll_element_into_view() reancorando um elemento JÁ VISÍVEL,
//     y=204 -> y=136, um salto indesejado) só pela API PÚBLICA do glintfx::UiLayer -- a nova
//     sobrecarga `scroll_element_into_view(const char* id, ScrollAlign align)`
//     (glintfx/include/glintfx/scroll_types.hpp), nunca Bootstrap/Engine/Rml::Element
//     diretamente.
//
//     Reusa a fixture compartilhada scroll_scene.rml/.rcss de scroll_sanity.cpp (ver o
//     cabeçalho daquele arquivo pro oráculo completo da cena: #scroller é um viewport border-box
//     120x100 sobre 30 itens de 30px, intervalo rolável [0,800]). Este arquivo soma MAIS um
//     oráculo em cima: a caixa local-de-conteúdo do #item-10 é [300, 330] -- exatamente o item
//     que a própria nota de mutation testing (B4) de scroll_sanity.cpp já estabeleceu dar dois
//     resultados DISCRIMINÁVEIS, não-saturados, para align_with_top=true/false (~300 / ~230),
//     motivo exato pelo qual é reusado aqui em vez de escolher um item novo.
//
//     (1) O PRÓPRIO BUG, reproduzido como asserção: com o viewport rolado para 250
//         (view=[250,350]), o #item-10 [300,330] JÁ está totalmente dentro -- begin_offset=50>=0
//         e end_offset=-20<=0 nos termos do próprio GetScrollOffsetDelta do RmlUi (Element.cpp),
//         exatamente a condição que Nearest e Adaptive tratam como "já visível, não rola".
//         scroll_element_into_view(id, ScrollAlign::Adaptive) deve deixar o scroll_top
//         INALTERADO -- esta é a asserção de delta-zero que substitui o caso y=204 do
//         consumidor (a medida dele de y=204 -> y=136 É um delta não-zero num elemento já
//         visível; este teste pina o delta em exatamente zero, o comportamento consertado).
//     (2) O CONSERTO DE FATO ROLA quando necessário: com o viewport em 0 (view=[0,100]), o
//         #item-10 [300,330] está totalmente FORA da área visível -- o Adaptive deve rolar,
//         caindo no ponto médio (300+230)/2 = 265 pelo ramo Adaptive do GetScrollOffsetDelta
//         (média dos oráculos Start/End abaixo, ele próprio um número derivado, não chutado).
//     (3) ScrollAlign::Start reproduz align_with_top=true da sobrecarga bool byte a byte (o
//         mesmo mapeamento pra Rml::ScrollAlignment::Start por baixo dos panos) -- provado por
//         comparação DIRETA A/B contra a sobrecarga bool pré-existente, não repetindo o mesmo
//         número mágico duas vezes. Da mesma forma, ScrollAlign::End reproduz
//         align_with_top=false.
//     (4) A PROVA ANTI-AMBIGUIDADE: uma chamada de um único argumento,
//         scroll_element_into_view(id), precisa continuar compilando e cair na sobrecarga bool
//         ANTIGA (align_with_top cai no default true) -- a sobrecarga de enum abaixo é
//         deliberadamente declarada SEM argumento default justamente pra isso continuar
//         inambíguo. A prova de verdade é este arquivo compilar (um conjunto de sobrecargas
//         ambíguo é erro de compilação, não falha em runtime); a asserção abaixo ainda pina o
//         oráculo da sobrecarga ANTIGA (item-29 satura no valor clampado 800, conforme o B3 de
//         scroll_sanity.cpp) como guarda de regressão em cima da prova de compile-time.
//     (5) Paridade de hardening com a sobrecarga bool (convenção AUD-TEC-5): id nulo / id vazio
//         "" / id desconhecido / nenhum-documento-carregado-ainda na sobrecarga NOVA retornam
//         todos false sem crashar, mesmo formato de guard de todo outro método de scroll deste
//         código-fonte.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cmath>
#include <cstdio>

static bool approx(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("scroll_align_host", 200, 200)) {
    std::puts("FAIL: host create");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = 200, .logical_height = 200});
  if (!ui.ok()) {
    std::puts("FAIL: ui attach");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // (5a) Hardening -- no document loaded yet, NEW overload must fail soft too.
  // ---------------------------------------------------------------------------
  if (ui.scroll_element_into_view("scroller", glintfx::ScrollAlign::Adaptive)) {
    std::puts("FAIL: scroll_into_view(align) before load");
    return 3;
  }

  if (!ui.load("scroll_scene.rml")) {
    std::puts("FAIL: load");
    return 4;
  }
  ui.update();
  ui.render();

  // ---------------------------------------------------------------------------
  // (5b) Hardening -- null id / empty id "" (AUD-TEC-5) / unknown id, NEW overload.
  // ---------------------------------------------------------------------------
  if (ui.scroll_element_into_view(nullptr, glintfx::ScrollAlign::Adaptive)) {
    std::puts("FAIL: scroll_into_view(nullptr, align)");
    return 5;
  }
  if (ui.scroll_element_into_view("", glintfx::ScrollAlign::Adaptive)) {
    std::puts("FAIL: scroll_into_view(\"\", align)");
    return 6;
  }
  if (ui.scroll_element_into_view("does-not-exist", glintfx::ScrollAlign::Adaptive)) {
    std::puts("FAIL: scroll_into_view(unknown id, align)");
    return 7;
  }

  // ---------------------------------------------------------------------------
  // (4) Anti-ambiguity proof: single-argument call still compiles (this TU building at all is
  //     the real proof) and dispatches to the OLD bool overload -- item-29's clamped-800
  //     oracle (scroll_sanity.cpp B3) as a regression guard on top of that.
  // ---------------------------------------------------------------------------
  if (!ui.scroll_element_into_view("item-29")) {
    std::puts("FAIL: scroll_into_view(item-29) [1-arg]");
    return 8;
  }
  float top = -1.f;
  if (!ui.get_element_scroll_top("scroller", top)) {
    std::puts("FAIL: get_scroll_top after 1-arg call");
    return 9;
  }
  if (!approx(top, 800.f, 1.5f)) {
    std::fprintf(stderr, "FAIL: scroll_top=%.2f expected ~800 (1-arg call must still hit the old bool overload)\n", top);
    return 10;
  }

  // ---------------------------------------------------------------------------
  // (3) ScrollAlign::Start / End reproduce the bool overload's true/false, by DIRECT A/B
  //     comparison (not by re-hardcoding the same magic numbers scroll_sanity.cpp already
  //     pins). Both calls are independent of the scroll_top in effect before them (see
  //     scroll_sanity.cpp B4's derivation note), so no reset is needed between them.
  // ---------------------------------------------------------------------------
  if (!ui.scroll_element_into_view("item-10", /*align_with_top=*/true)) {
    std::puts("FAIL: scroll_into_view(item-10, true) [bool overload]");
    return 11;
  }
  float top_bool_true = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_bool_true)) {
    std::puts("FAIL: get_scroll_top after bool(true)");
    return 12;
  }

  if (!ui.scroll_element_into_view("item-10", glintfx::ScrollAlign::Start)) {
    std::puts("FAIL: scroll_into_view(item-10, ScrollAlign::Start)");
    return 13;
  }
  float top_align_start = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_align_start)) {
    std::puts("FAIL: get_scroll_top after ScrollAlign::Start");
    return 14;
  }

  if (!approx(top_bool_true, top_align_start, 0.5f)) {
    std::fprintf(stderr,
                 "FAIL: ScrollAlign::Start (%.2f) did not reproduce align_with_top=true (%.2f)\n",
                 top_align_start, top_bool_true);
    return 15;
  }
  if (!approx(top_align_start, 300.f, 1.5f)) {
    std::fprintf(stderr, "FAIL: ScrollAlign::Start landed on %.2f, expected ~300 (scroll_sanity.cpp B4 oracle)\n",
                 top_align_start);
    return 16;
  }

  if (!ui.scroll_element_into_view("item-10", /*align_with_top=*/false)) {
    std::puts("FAIL: scroll_into_view(item-10, false) [bool overload]");
    return 17;
  }
  float top_bool_false = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_bool_false)) {
    std::puts("FAIL: get_scroll_top after bool(false)");
    return 18;
  }

  if (!ui.scroll_element_into_view("item-10", glintfx::ScrollAlign::End)) {
    std::puts("FAIL: scroll_into_view(item-10, ScrollAlign::End)");
    return 19;
  }
  float top_align_end = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_align_end)) {
    std::puts("FAIL: get_scroll_top after ScrollAlign::End");
    return 20;
  }

  if (!approx(top_bool_false, top_align_end, 0.5f)) {
    std::fprintf(stderr,
                 "FAIL: ScrollAlign::End (%.2f) did not reproduce align_with_top=false (%.2f)\n",
                 top_align_end, top_bool_false);
    return 21;
  }
  if (!approx(top_align_end, 230.f, 1.5f)) {
    std::fprintf(stderr, "FAIL: ScrollAlign::End landed on %.2f, expected ~230 (scroll_sanity.cpp B4 oracle)\n",
                 top_align_end);
    return 22;
  }

  // ---------------------------------------------------------------------------
  // (2) Adaptive DOES scroll when the element is out of view: viewport at 0 (view=[0,100]),
  //     #item-10 [300,330] fully outside it -- must land on the midpoint 265.
  // ---------------------------------------------------------------------------
  if (!ui.set_element_scroll_top("scroller", 0.f)) {
    std::puts("FAIL: reset scroll_top to 0");
    return 23;
  }
  if (!ui.scroll_element_into_view("item-10", glintfx::ScrollAlign::Adaptive)) {
    std::puts("FAIL: scroll_into_view(item-10, Adaptive) out-of-view");
    return 24;
  }
  float top_adaptive_scrolled = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_adaptive_scrolled)) {
    std::puts("FAIL: get_scroll_top after Adaptive out-of-view");
    return 25;
  }
  if (!approx(top_adaptive_scrolled, 265.f, 1.5f)) {
    std::fprintf(stderr,
                 "FAIL: Adaptive out-of-view landed on %.2f, expected ~265 (midpoint of the 300/230 Start/End oracles)\n",
                 top_adaptive_scrolled);
    return 26;
  }
  if (approx(top_adaptive_scrolled, 0.f, 1.5f)) {
    std::puts("FAIL: Adaptive out-of-view did not scroll at all (stayed at baseline 0)");
    return 27;
  }

  // ---------------------------------------------------------------------------
  // (1) THE BUG, reproduced and fixed: element ALREADY fully visible (viewport at 250,
  //     view=[250,350], #item-10 [300,330] fully inside) -- Adaptive must be a delta-zero no-op,
  //     the exact fix for the consumer's measured y=204 -> y=136 re-anchor.
  // ---------------------------------------------------------------------------
  if (!ui.set_element_scroll_top("scroller", 250.f)) {
    std::puts("FAIL: set scroll_top to 250");
    return 28;
  }
  float top_before_visible_adaptive = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_before_visible_adaptive)) {
    std::puts("FAIL: get_scroll_top baseline=250");
    return 29;
  }
  if (!approx(top_before_visible_adaptive, 250.f, 0.5f)) {
    std::fprintf(stderr, "FAIL: baseline scroll_top=%.2f expected ~250\n", top_before_visible_adaptive);
    return 30;
  }

  if (!ui.scroll_element_into_view("item-10", glintfx::ScrollAlign::Adaptive)) {
    std::puts("FAIL: scroll_into_view(item-10, Adaptive) already-visible");
    return 31;
  }
  float top_after_visible_adaptive = -1.f;
  if (!ui.get_element_scroll_top("scroller", top_after_visible_adaptive)) {
    std::puts("FAIL: get_scroll_top after Adaptive already-visible");
    return 32;
  }

  const float delta = top_after_visible_adaptive - top_before_visible_adaptive;
  if (!approx(delta, 0.f, 0.5f)) {
    std::fprintf(stderr,
                 "FAIL: Adaptive on an already-visible element moved scroll_top by %.2f "
                 "(before=%.2f after=%.2f) -- expected delta 0, this IS the consumer-measured bug "
                 "(y=204 -> y=136) reproduced\n",
                 delta, top_before_visible_adaptive, top_after_visible_adaptive);
    return 33;
  }

  if (!ui.ok()) {
    std::puts("FAIL: ok() false after scroll-align sequence");
    return 34;
  }

  std::puts("scroll_align_sanity: PASS");
  return 0;
}
