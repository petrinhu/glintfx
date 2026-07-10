// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_own_kern_sanity -- L1.19-FONTENG/L1.20-FONTFLIP kerning-APPLICATION oracle.
//
//     Proves glintfx's own font engine (FontEngineOwn::IterateGlyphs(), font_engine_own.cpp)
//     actually FOLDS the SOV-SFNT `kern` table adjustment (glx_sfnt_kern(), include/core/sfnt.h)
//     into the pen position -- not merely that SOV-SFNT can READ a kern value (that is already
//     proven, independently, by the Camada 0 test that landed with commit 7d3d6d4). This is a
//     NUMERIC width oracle, not a visual/pixel one (unlike fonteng_own_sanity.cpp/
//     fonteng_own_embed_sanity.cpp, which only prove glyphs render at all) -- deterministic,
//     not flaky, no Xvfb pixel-statistics tolerance needed.
//
//     ORACLE, both pairs measured through UiLayer::get_element_box(id).w on
//     tests/fonteng_kern_scene.rml's `display: inline-block` spans (shrink-to-fit content width,
//     zero padding/border/margin -- border-box width IS the text's own measured width):
//
//       (1) KERNED pair -- "AV" ('A' immediately before 'V' -- Open Sans' own `kern` table DOES
//           have this pair, confirmed against tests/assets/OpenSans-Regular.ttf by a throwaway
//           probe program built during this ticket: kern(A,V) = -82 FUnits, units_per_em=2048).
//           At this scene's 48px font-size that is -82 * 48/2048 = -1.921875 px -- so
//           width("AV") must be MEASURABLY less than width("A")+width("V"), specifically by AT
//           LEAST 2px (the same probe's own numbers: width("A")=30, width("V")=29,
//           width("AV")=57 -- a 2px gap). A `>= 2` floor (not `> 0`) is deliberate: it must beat
//           the +-1px rounding-carry noise check (2) below documents, so this check can only pass
//           from a REAL kern adjustment, never from float-accumulation rounding alone.
//       (2) UN-KERNED CONTROL pair -- "HH" ('H' immediately before 'H' -- confirmed by the same
//           probe to have NO `kern` table entry, kern==0). width("HH") must be
//           width("H")+width("H") give or take AT MOST 1px. That +-1px tolerance -- not exact
//           equality -- is not a concession to this engine's imprecision: it is the SAME
//           unavoidable "round the total once, not each part" artifact GetStringWidth()/
//           get_element_box() would show even with kern==0 identically, because
//           IterateGlyphs() accumulates `pen` as an unrounded float across the whole string and
//           only the FINAL total is rounded (see IterateGlyphs()'s own doc-comment in
//           font_engine_own.hpp) -- summing two INDEPENDENTLY-rounded single-glyph widths can
//           differ from the twice-as-long string's OWN single rounding by up to 1px purely from
//           where the fractional carry falls, with no kerning involved at all (confirmed by the
//           same probe: advance("H")=35.4141px unrounded, round()=35px; two of them un-rounded
//           sum to 70.8282px, which rounds to 71 -- a 1px difference from 2*35=70, despite
//           kern(H,H)==0 exactly). Asserting `== 0` here would make this test QUIETLY font/
//           glyph-choice-fragile (liable to flip on any future font-asset swap that happens to
//           land the pair's advances the other side of that fractional boundary) without proving
//           anything more about kerning than the `<= 1` bound already does.
//
//     Registered in tests/CMakeLists.txt's `if(GLINTFX_OWN_FONT_ENGINE)` block (same one as
//     fonteng_own_embed_sanity.cpp), OUTSIDE `if(GLINTFX_BACKEND_GLFW)`, via UiLayer + the
//     ${_embed_dep} target -- builds and runs identically whether GLINTFX_BACKEND_GLFW is ON or
//     OFF, so this one file exercises the kerning fold on BOTH the GLFW-hosted-App CI path and
//     GusWorld's own embed-only (UiLayer, no glintfx::App at all) consumption path.
// PT: fonteng_own_kern_sanity -- oráculo de APLICAÇÃO de kerning do L1.19-FONTENG/
//     L1.20-FONTFLIP.
//
//     Prova que o motor de fonte próprio do glintfx (FontEngineOwn::IterateGlyphs(),
//     font_engine_own.cpp) de fato DOBRA o ajuste da tabela `kern` do SOV-SFNT (glx_sfnt_kern(),
//     include/core/sfnt.h) na posição da pena -- não só que o SOV-SFNT consegue LER um valor de
//     kern (isso já está provado, independentemente, pelo teste da Camada 0 que chegou junto do
//     commit 7d3d6d4). Este é um oráculo NUMÉRICO de largura, não visual/de-pixel (diferente de
//     fonteng_own_sanity.cpp/fonteng_own_embed_sanity.cpp, que só provam que glyphs renderizam de
//     algum jeito) -- determinístico, não-flaky, sem precisar de tolerância estatística de pixel
//     do Xvfb.
//
//     ORÁCULO, os dois pares medidos via UiLayer::get_element_box(id).w nos spans
//     `display: inline-block` de tests/fonteng_kern_scene.rml (largura de encolher-para-caber do
//     conteúdo, padding/borda/margem zero -- a largura border-box É a própria largura medida do
//     texto):
//
//       (1) Par COM KERN -- "AV" ('A' imediatamente antes de 'V' -- a própria tabela `kern` da
//           Open Sans TEM este par, confirmado contra tests/assets/OpenSans-Regular.ttf por um
//           programa-sonda descartável construído durante esta tarefa: kern(A,V) = -82 FUnits,
//           units_per_em=2048). Neste font-size de 48px da cena isso é -82 * 48/2048 =
//           -1.921875 px -- então width("AV") precisa medir MENSURAVELMENTE menos que
//           width("A")+width("V"), especificamente em PELO MENOS 2px (os próprios números da
//           sonda: width("A")=30, width("V")=29, width("AV")=57 -- um vão de 2px). Um piso `>= 2`
//           (não `> 0`) é deliberado: precisa vencer o ruído de +-1px de carregamento de
//           arredondamento que o cheque (2) abaixo documenta, então este cheque só passa por um
//           ajuste de kern REAL, nunca só por acumulação de ponto flutuante.
//       (2) Par de CONTROLE SEM KERN -- "HH" ('H' imediatamente antes de 'H' -- confirmado pela
//           mesma sonda como SEM entrada na tabela `kern`, kern==0). width("HH") precisa ser
//           width("H")+width("H") com folga de NO MÁXIMO 1px. Essa tolerância de +-1px -- não
//           igualdade exata -- não é uma concessão à imprecisão deste motor: é o MESMO artefato
//           inevitável de "arredondar o total uma vez, não cada parte" que GetStringWidth()/
//           get_element_box() mostrariam identicamente mesmo com kern==0, porque IterateGlyphs()
//           acumula `pen` como float não-arredondado durante a string inteira e só o total FINAL
//           é arredondado (ver o doc-comment próprio de IterateGlyphs() em font_engine_own.hpp)
//           -- somar duas larguras de glyph único arredondadas INDEPENDENTEMENTE pode diferir da
//           PRÓPRIA rodada única de arredondamento da string duas-vezes-mais-longa em até 1px,
//           puramente por onde o carregamento fracionário cai, sem kerning nenhum envolvido
//           (confirmado pela mesma sonda: advance("H")=35.4141px não-arredondado, round()=35px;
//           duas delas não-arredondadas somam 70.8282px, que arredonda pra 71 -- uma diferença de
//           1px de 2*35=70, apesar de kern(H,H)==0 exato). Afirmar `== 0` aqui tornaria este
//           teste SILENCIOSAMENTE frágil a fonte/escolha-de-glyph (sujeito a virar numa futura
//           troca de asset de fonte que por acaso pouse os avanços do par do outro lado dessa
//           fronteira fracionária) sem provar nada a mais sobre kerning do que o limite `<= 1` já
//           prova.
//
//     Registrado no bloco `if(GLINTFX_OWN_FONT_ENGINE)` do tests/CMakeLists.txt (o mesmo de
//     fonteng_own_embed_sanity.cpp), FORA de `if(GLINTFX_BACKEND_GLFW)`, via UiLayer + o alvo
//     ${_embed_dep} -- builda e roda identicamente com GLINTFX_BACKEND_GLFW ON ou OFF, então este
//     único arquivo exercita a dobra de kerning TANTO no caminho de CI hospedado-por-GLFW-App
//     QUANTO no próprio caminho de consumo embed-only do GusWorld (UiLayer, sem glintfx::App
//     nenhum).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cmath>
#include <cstdio>

int main() {
  constexpr int W = 320, H = 120;

  // EN: Host window (GL context provider) -- same fixture shape as fonteng_own_embed_sanity.cpp
  //     and every other embed test in this suite. A failed host.create() is a FAIL, not a SKIP
  //     (Xvfb provides a real, if software, GL context in CI).
  // PT: Janela host (provê o contexto GL) -- mesmo formato de fixture de
  //     fonteng_own_embed_sanity.cpp e todo outro teste embed desta suíte. Um host.create() falho
  //     é FAIL, não SKIP (Xvfb provê um contexto GL real, ainda que por software, em CI).
  glintfx::WindowGlfw host;
  if (!host.create("fonteng_own_kern_sanity_host", W, H)) {
    std::puts("fonteng_own_kern_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_own_kern_sanity FAIL: ui attach failed");
    return 2;
  }

  if (!ui.load("fonteng_kern_scene.rml")) {
    std::fputs("fonteng_own_kern_sanity FAIL: load() returned false\n", stderr);
    return 3;
  }

  // EN: Two warm-up update+render cycles -- same convention as every other embed test in this
  //     suite (frame 1 settles RmlUi's internal layout allocation, frame 2 is the stable state
  //     inspected below). This test does not read pixels back, but get_element_box() reads
  //     RmlUi's OWN computed layout box, which is only trustworthy once layout has actually run
  //     -- render() (not just update()) is what this suite's other tests rely on to guarantee
  //     that, so the same two-cycle shape is kept here for consistency, not cargo-culted.
  // PT: Dois ciclos de aquecimento update+render -- mesma convenção de todo outro teste embed
  //     desta suíte (ciclo 1 estabiliza a alocação interna de layout do RmlUi, ciclo 2 é o
  //     estado estável inspecionado abaixo). Este teste não lê pixels de volta, mas
  //     get_element_box() lê a PRÓPRIA caixa de layout computada do RmlUi, que só é confiável
  //     depois que o layout de fato rodou -- render() (não só update()) é o que os outros testes
  //     desta suíte usam pra garantir isso, então a mesma forma de dois ciclos é mantida aqui por
  //     consistência, não copiada sem motivo.
  ui.update(); ui.render();
  ui.update(); ui.render();

  const glintfx::ElementBox box_av = ui.get_element_box("av");
  const glintfx::ElementBox box_a  = ui.get_element_box("a");
  const glintfx::ElementBox box_v  = ui.get_element_box("v");
  const glintfx::ElementBox box_hh = ui.get_element_box("hh");
  const glintfx::ElementBox box_h1 = ui.get_element_box("h1");
  const glintfx::ElementBox box_h2 = ui.get_element_box("h2");

  if (!box_av.found || !box_a.found || !box_v.found ||
      !box_hh.found || !box_h1.found || !box_h2.found) {
    std::fprintf(stderr,
        "fonteng_own_kern_sanity FAIL [0] get_element_box found=(av=%d,a=%d,v=%d,hh=%d,h1=%d,h2=%d) "
        "-- expected all true (ids missing from fonteng_kern_scene.rml?)\n",
        box_av.found, box_a.found, box_v.found, box_hh.found, box_h1.found, box_h2.found);
    return 4;
  }

  const float width_av = box_av.w;
  const float width_a  = box_a.w;
  const float width_v  = box_v.w;
  const float width_hh = box_hh.w;
  const float width_h1 = box_h1.w;
  const float width_h2 = box_h2.w;

  const float sum_av = width_a + width_v;
  const float sum_hh = width_h1 + width_h2;
  const float kern_gap = sum_av - width_av;         // expected ~2px (>= 2 asserted below).
  const float control_gap = std::fabs(sum_hh - width_hh); // expected <= 1px (rounding noise only).

  std::printf("fonteng_own_kern_sanity: width(AV)=%.3f width(A)=%.3f width(V)=%.3f "
              "sum(A+V)=%.3f kern_gap=%.3f | "
              "width(HH)=%.3f width(H1)=%.3f width(H2)=%.3f sum(H1+H2)=%.3f control_gap=%.3f\n",
              width_av, width_a, width_v, sum_av, kern_gap,
              width_hh, width_h1, width_h2, sum_hh, control_gap);

  int failures = 0;

  // EN: Check 1 -- KERNED pair: width("AV") strictly, measurably less than width("A")+width("V")
  //     -- the actual proof that IterateGlyphs() applies the kern adjustment (not just that
  //     SOV-SFNT can read one). See this file's header for the derivation of the `>= 2` floor
  //     (Open Sans' own kern(A,V) = -82 FUnits at 48px = -1.921875px, beating the +-1px
  //     rounding-carry noise check 2 documents).
  // PT: Cheque 1 -- par COM KERN: width("AV") estrita, mensuravelmente menor que
  //     width("A")+width("V") -- a prova de fato de que IterateGlyphs() aplica o ajuste de kern
  //     (não só que o SOV-SFNT consegue ler um). Ver o cabeçalho deste arquivo pra derivação do
  //     piso `>= 2` (o próprio kern(A,V) da Open Sans = -82 FUnits a 48px = -1.921875px, vencendo
  //     o ruído de +-1px de carregamento de arredondamento que o cheque 2 documenta).
  if (kern_gap < 2.0f) {
    std::fprintf(stderr,
        "fonteng_own_kern_sanity FAIL [1] kern_gap=%.3f < 2.0 -- width(AV)=%.3f is not "
        "measurably less than width(A)+width(V)=%.3f; kerning does not look APPLIED "
        "(SOV-SFNT's glx_sfnt_kern(A,V) is known to return -82 FUnits for this exact .ttf)\n",
        kern_gap, width_av, sum_av);
    ++failures;
  }

  // EN: Check 2 -- UN-KERNED control pair: width("HH") matches width("H")+width("H") within the
  //     +-1px rounding-carry tolerance this file's header derives (NOT exact equality -- see
  //     that derivation for why `== 0` would be font/glyph-fragile without proving more).
  // PT: Cheque 2 -- par de controle SEM KERN: width("HH") bate com width("H")+width("H") dentro
  //     da tolerância de +-1px de carregamento de arredondamento que o cabeçalho deste arquivo
  //     deriva (NÃO igualdade exata -- ver aquela derivação pro motivo de `== 0` seria frágil a
  //     fonte/glyph sem provar mais nada).
  if (control_gap > 1.0f) {
    std::fprintf(stderr,
        "fonteng_own_kern_sanity FAIL [2] control_gap=%.3f > 1.0 -- width(HH)=%.3f drifted too "
        "far from width(H)+width(H)=%.3f for a pair SOV-SFNT's glx_sfnt_kern(H,H) is known to "
        "return 0 (no kern entry) for this exact .ttf -- looks like an unwanted adjustment is "
        "being applied where none should be\n",
        control_gap, width_hh, sum_hh);
    ++failures;
  }

  // EN: Check 3 -- ok() stays true after the full render sequence (embed-mode-specific hygiene
  //     check every other UiLayer embed test in this suite also performs).
  // PT: Cheque 3 -- ok() permanece true após a sequência completa de render (checagem de higiene
  //     específica do embed mode que todo outro teste embed de UiLayer desta suíte também faz).
  if (!ui.ok()) {
    std::fputs("fonteng_own_kern_sanity FAIL [3] ok() false after render sequence\n", stderr);
    ++failures;
  }

  if (failures == 0) {
    std::fputs("fonteng_own_kern_sanity: PASS\n", stdout);
    return 0;
  }
  std::fprintf(stderr, "fonteng_own_kern_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
