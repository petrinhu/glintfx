// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_ab_compare -- L1.20-FONTFLIP (FT-F4) A/B safety net.
//
//     THE SAFETY NET FOR "THE FLIP". L1.20-FONTFLIP will eventually make glintfx's own
//     SOV-SFNT/SOV-RAST font engine the DEFAULT and remove FreeType. Before that irreversible
//     step, this test pins a NUMERIC, side-by-side comparison of the two engines rendering the
//     EXACT same document, so a gross regression in the own engine (a missing glyph, a 2x-wrong
//     advance, a broken pt-br composite-accent outline) is caught mechanically instead of by eye.
//
//     ONE BUILD, BOTH ENGINES. This runs in a single GLINTFX_OWN_FONT_ENGINE=ON build (NO second
//     build, NO RmlUi rebuild). In an ON build BOTH engines are present in the binary: RmlUi's
//     FreeType default (RMLUI_FONT_ENGINE=freetype stays the CMake default -- see
//     glintfx/CMakeLists.txt) AND glintfx's own engine, which Bootstrap::init() normally installs
//     over it at runtime via Rml::SetFontEngineInterface(). The engine is selected PER SESSION by
//     the src-internal test hook glintfx::own_font_engine_ab_bypass() (font_engine_own.hpp):
//       - bypass=false -> Bootstrap::init() installs FontEngineOwn        => OWN engine active.
//       - bypass=true  -> Bootstrap::init() skips the install; RmlUi's
//                          Initialise() then creates its FreeType default  => FREETYPE active.
//     Two full UiLayer sessions run sequentially in this one process. That is provably clean:
//     RmlUi's Shutdown() resets its global font_interface to null and Initialise() recreates the
//     FreeType default only when none was set (confirmed reading the pinned RmlUi
//     Source/Core/Core.cpp), so each session independently re-selects whichever engine its
//     bypass value asks for, with no cross-contamination.
//
//     UiLayer (embed) PATH, so this ONE file builds and runs in BOTH lib configs -- GLFW=ON and
//     embed-only GLFW=OFF -- registered OUTSIDE if(GLINTFX_BACKEND_GLFW) via ${_embed_dep}, the
//     same pattern as fonteng_own_kern_sanity.cpp / fonteng_own_embed_sanity.cpp. A single hidden
//     GLFW host window provides the GL context and is REUSED across both sessions (only RmlUi is
//     re-initialised between them, never GLFW), so the two engines render into the identical GL
//     environment -- the fairest possible A/B.
//
//     METRICS (per span, both quantifiable and reported numerically -- never "looks the same"):
//       (A) ADVANCE WIDTH -- UiLayer::get_element_box(id).w. Each span is `display: inline-block`
//           with zero padding/border/margin (fonteng_ab_scene.rml), so .w IS the string's
//           shrink-to-fit advance width, i.e. exactly what each engine's GetStringWidth() summed
//           from the font's hmtx advances. Both engines read hmtx for the SAME glyph ids, so a
//           correct own engine must land within a small tolerance of FreeType. A missing glyph
//           (advance 0) or a 2x-wrong advance blows past that tolerance.
//       (B) INK COVERAGE -- count of "ink" (near-white) pixels inside that same span's bounding
//           box, from a glReadPixels of each engine's rendered frame. Width alone cannot catch a
//           broken composite-accent OUTLINE whose advance is still correct (advance comes from
//           hmtx regardless of the outline) -- the accent mark could silently vanish while the
//           width stays right. Ink coverage catches that: an accented span's ink would collapse
//           toward the base-letters-only count, diverging grossly from FreeType.
//
//     TOLERANCES (justified; the two engines are NOT pixel-identical -- the goal is to flag GROSS
//     divergence, not to demand equality):
//       (A) width: |w_own - w_ft| / w_ft <= 0.12 (12%). Realistic engine difference on these
//           strings is ~2-4% (FreeType rounds each glyph's advance to a pixel then sums; the own
//           engine sums fractional advances and rounds once -- bounded by ~0.5px per glyph). 12%
//           is generous headroom over that yet still below the >=~16% shortfall a single missing
//           glyph causes on the 6-glyph accented spans, and far below a 2x (100%) advance error.
//       (B) ink: 0.40 <= ink_own / ink_ft <= 2.5. Deliberately COARSE -- anti-aliasing/hinting
//           differences legitimately move raw coverage counts by tens of percent, so a tight
//           bound would be flaky. This band still catches the real failure shapes: an accent that
//           renders NOTHING drives the ratio toward 0; a coverage-blit bug that fills solid drives
//           it far above 2.5.
//
//     Deterministic (no flakiness): get_element_box() widths are pure layout math; plain
//     un-blurred text ink counts are stable under llvmpipe (the same statistical-oracle basis
//     render_sanity.cpp / fonteng_own_embed_sanity.cpp already rely on). No sleeps, no retries.
// PT: fonteng_ab_compare -- rede de segurança do A/B do L1.20-FONTFLIP (FT-F4).
//
//     A REDE DE SEGURANÇA PARA "O FLIP". O L1.20-FONTFLIP vai eventualmente tornar o motor de
//     fonte próprio SOV-SFNT/SOV-RAST do glintfx o DEFAULT e remover o FreeType. Antes desse passo
//     irreversível, este teste fixa uma comparação NUMÉRICA, lado-a-lado, dos dois motores
//     renderizando o MESMO documento, para que uma regressão grosseira no motor próprio (glyph
//     faltando, avanço 2x errado, outline de acento composto pt-br quebrado) seja pega
//     mecanicamente em vez de a olho.
//
//     UM BUILD, DOIS MOTORES. Roda num único build GLINTFX_OWN_FONT_ENGINE=ON (SEM segundo build,
//     SEM rebuild do RmlUi). Num build ON os DOIS motores estão no binário: o FreeType default do
//     RmlUi (RMLUI_FONT_ENGINE=freetype continua o default de CMake -- ver glintfx/CMakeLists.txt)
//     E o motor próprio do glintfx, que o Bootstrap::init() normalmente instala por cima em
//     runtime via Rml::SetFontEngineInterface(). O motor é selecionado POR SESSÃO pelo hook
//     src-interno de teste glintfx::own_font_engine_ab_bypass() (font_engine_own.hpp):
//       - bypass=false -> Bootstrap::init() instala o FontEngineOwn        => motor PRÓPRIO ativo.
//       - bypass=true  -> Bootstrap::init() pula a instalação; o Initialise()
//                          do RmlUi então cria seu FreeType default        => FREETYPE ativo.
//     Duas sessões UiLayer completas rodam sequencialmente neste único processo. Isso é
//     comprovadamente limpo: o Shutdown() do RmlUi reseta seu font_interface global pra nulo e o
//     Initialise() recria o FreeType default só quando nenhum foi setado (confirmado lendo o
//     Source/Core/Core.cpp pinado do RmlUi), então cada sessão re-seleciona independentemente
//     qualquer motor que seu bypass pedir, sem contaminação cruzada.
//
//     Caminho UiLayer (embed), então este ÚNICO arquivo builda e roda nos DOIS configs da lib --
//     GLFW=ON e embed-only GLFW=OFF -- registrado FORA de if(GLINTFX_BACKEND_GLFW) via
//     ${_embed_dep}, mesmo padrão de fonteng_own_kern_sanity.cpp / fonteng_own_embed_sanity.cpp.
//     Uma única janela host GLFW oculta provê o contexto GL e é REUSADA entre as duas sessões (só
//     o RmlUi é re-inicializado entre elas, nunca o GLFW), então os dois motores renderizam no
//     ambiente GL idêntico -- o A/B mais justo possível.
//
//     MÉTRICAS (por span, ambas quantificáveis e reportadas numericamente -- nunca "parece
//     igual"):
//       (A) LARGURA DE AVANÇO -- UiLayer::get_element_box(id).w. Cada span é
//           `display: inline-block` com padding/borda/margem zero (fonteng_ab_scene.rml), então .w
//           É a largura de avanço de encolher-para-caber da string, ou seja exatamente o que o
//           GetStringWidth() de cada motor somou dos avanços hmtx da fonte. Os dois motores leem
//           hmtx pros MESMOS glyph ids, então um motor próprio correto precisa cair dentro de uma
//           pequena tolerância do FreeType. Um glyph faltando (avanço 0) ou um avanço 2x errado
//           estoura essa tolerância.
//       (B) COBERTURA DE TINTA -- contagem de pixels "tinta" (quase-branco) dentro da mesma
//           bounding box do span, de um glReadPixels do frame renderizado de cada motor. A largura
//           sozinha não pega um OUTLINE de acento composto quebrado cujo avanço ainda está certo
//           (o avanço vem do hmtx independente do outline) -- o acento poderia sumir em silêncio
//           enquanto a largura fica certa. A cobertura de tinta pega isso: a tinta de um span
//           acentuado desabaria em direção à contagem só-das-letras-base, divergindo grosseiramente
//           do FreeType.
//
//     TOLERÂNCIAS (justificadas; os dois motores NÃO são pixel-idênticos -- o objetivo é flagrar
//     divergência GROSSEIRA, não exigir igualdade):
//       (A) largura: |w_own - w_ft| / w_ft <= 0.12 (12%). A diferença realista entre motores
//           nessas strings é ~2-4% (o FreeType arredonda o avanço de cada glyph pra um pixel e
//           soma; o motor próprio soma avanços fracionários e arredonda uma vez -- limitado a
//           ~0.5px por glyph). 12% é folga generosa sobre isso mas ainda abaixo do déficit de
//           >=~16% que um único glyph faltando causa nos spans acentuados de 6 glyphs, e muito
//           abaixo de um erro de avanço 2x (100%).
//       (B) tinta: 0.40 <= ink_own / ink_ft <= 2.5. Deliberadamente GROSSEIRA -- diferenças de
//           anti-aliasing/hinting movem legitimamente as contagens brutas de cobertura em dezenas
//           de por cento, então um limite apertado seria flaky. Esta banda ainda pega as formas
//           reais de falha: um acento que renderiza NADA leva o ratio pra perto de 0; um bug de
//           blit de cobertura que preenche sólido leva-o bem acima de 2.5.
//
//     Determinístico (sem flakiness): as larguras de get_element_box() são pura matemática de
//     layout; as contagens de tinta de texto simples sem blur são estáveis sob llvmpipe (a mesma
//     base de oráculo estatístico que render_sanity.cpp / fonteng_own_embed_sanity.cpp já usam).
//     Sem sleeps, sem retries.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// EN: A/B engine-selection hook, forward-declared locally rather than #include-ing
//     font_engine_own.hpp -- that header transitively pulls Layer 0's "core/raster.h"/"core/sfnt.h"
//     on an include path CMake only wires for the font_engine_own.cpp translation unit inside the
//     lib (set_source_files_properties in glintfx/CMakeLists.txt), never for a test target. The
//     symbol has external linkage in the glintfx static lib (compiled when GLINTFX_OWN_FONT_ENGINE
//     =ON, which is exactly when this test is built), so a plain declaration is all the linker
//     needs. See its full contract in font_engine_own.hpp.
// PT: Hook de seleção de motor A/B, forward-declarado localmente em vez de #include-ar
//     font_engine_own.hpp -- esse header puxa transitivamente "core/raster.h"/"core/sfnt.h" da
//     Camada 0 num include path que o CMake só conecta pra unidade de tradução
//     font_engine_own.cpp dentro da lib (set_source_files_properties em glintfx/CMakeLists.txt),
//     nunca pra um alvo de teste. O símbolo tem external linkage na lib estática glintfx
//     (compilada quando GLINTFX_OWN_FONT_ENGINE=ON, que é exatamente quando este teste é buildado),
//     então uma declaração simples é tudo que o linker precisa. Ver o contrato completo em
//     font_engine_own.hpp.
namespace glintfx {
bool& own_font_engine_ab_bypass();
}

namespace {

constexpr int W = 640, H = 360;

// EN: The spans measured, in fonteng_ab_scene.rml order. accent_* / word_* exercise the pt-br
//     composite-accent path (all codepoints inside FontEngineOwn's BAKE_SET).
// PT: Os spans medidos, na ordem de fonteng_ab_scene.rml. accent_* / word_* exercitam o caminho
//     de acento composto pt-br (todos os codepoints dentro do BAKE_SET do FontEngineOwn).
const char* const kIds[] = {
    "ascii_caps", "ascii_low", "accent_low", "accent_up", "word_coracao", "word_acao",
};
constexpr int kN = static_cast<int>(sizeof(kIds) / sizeof(kIds[0]));

struct EngineMetrics {
  bool  found[kN] = {};
  float width[kN] = {};   // EN: get_element_box(id).w  PT: get_element_box(id).w
  long  ink[kN]   = {};   // EN: ink pixels inside the span's bbox  PT: pixels de tinta na bbox
};

// EN: glReadPixels' origin is bottom-left; get_element_box() (and this codebase's geometry) is
//     top-left, y-down -- flip the readback buffer in place so both agree, exactly as
//     fonteng_own_embed_sanity.cpp does (see its doc-comment for the full rationale).
// PT: A origem do glReadPixels é bottom-left; get_element_box() (e a geometria deste codebase) é
//     top-left, y-para-baixo -- inverte o buffer de readback in-place para os dois concordarem,
//     exatamente como fonteng_own_embed_sanity.cpp faz (ver seu doc-comment pro racional).
void flip_to_top_down(std::vector<unsigned char>& px, int w, int h) {
  const size_t row_bytes = static_cast<size_t>(w) * 3;
  std::vector<unsigned char> tmp(row_bytes);
  for (int top = 0, bottom = h - 1; top < bottom; ++top, --bottom) {
    unsigned char* rt = px.data() + static_cast<size_t>(top) * row_bytes;
    unsigned char* rb = px.data() + static_cast<size_t>(bottom) * row_bytes;
    std::copy(rt, rt + row_bytes, tmp.data());
    std::copy(rb, rb + row_bytes, rt);
    std::copy(tmp.data(), tmp.data() + row_bytes, rb);
  }
}

// EN: Counts "ink" pixels (any channel > threshold) inside the rectangle [x,x+w) x [y,y+h),
//     clamped to the frame -- px is TOP-DOWN RGB (flip_to_top_down already applied).
// PT: Conta pixels "tinta" (algum canal > threshold) dentro do retângulo [x,x+w) x [y,y+h),
//     recortado ao frame -- px é RGB TOP-DOWN (flip_to_top_down já aplicado).
long count_ink_rect(const std::vector<unsigned char>& px, int fw, int fh,
                    float rx, float ry, float rw, float rh, int threshold = 100) {
  int x0 = std::max(0, static_cast<int>(std::floor(rx)));
  int y0 = std::max(0, static_cast<int>(std::floor(ry)));
  int x1 = std::min(fw, static_cast<int>(std::ceil(rx + rw)));
  int y1 = std::min(fh, static_cast<int>(std::ceil(ry + rh)));
  long count = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * fw + x) * 3;
      if (px[i] > threshold || px[i + 1] > threshold || px[i + 2] > threshold) ++count;
    }
  }
  return count;
}

// EN: Runs ONE full UiLayer session with the requested engine (bypass=false -> own,
//     bypass=true -> FreeType), on the already-current GL context of `host`. Renders the scene,
//     reads back every span's advance width and per-span ink coverage into `out`. The UiLayer is
//     destroyed at return (Rml::Shutdown resets the global font_interface, readying the next
//     session's engine re-selection). Returns 0 on success, non-zero on a fixture failure.
// PT: Roda UMA sessão UiLayer completa com o motor pedido (bypass=false -> próprio,
//     bypass=true -> FreeType), no contexto GL já-corrente do `host`. Renderiza a cena, lê de
//     volta a largura de avanço de cada span e a cobertura de tinta por-span em `out`. A UiLayer é
//     destruída no retorno (Rml::Shutdown reseta o font_interface global, preparando a
//     re-seleção de motor da próxima sessão). Retorna 0 no sucesso, não-zero numa falha de fixture.
int measure(bool bypass, const char* label, EngineMetrics& out) {
  glintfx::own_font_engine_ab_bypass() = bypass;

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::fprintf(stderr, "fonteng_ab_compare FAIL: UiLayer attach failed (%s)\n", label);
    return 10;
  }

  // EN: Deterministic black baseline before load()/render() (same as fonteng_own_embed_sanity).
  // PT: Baseline preto determinístico antes de load()/render() (igual fonteng_own_embed_sanity).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_ab_scene.rml")) {
    std::fprintf(stderr, "fonteng_ab_compare FAIL: load() returned false (%s)\n", label);
    return 11;
  }

  // EN: Two warm-up frames -- suite convention (frame 1 settles layout, frame 2 is stable).
  // PT: Dois frames de aquecimento -- convenção da suíte (frame 1 estabiliza layout, frame 2
  //     estável).
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  for (int i = 0; i < kN; ++i) {
    const glintfx::ElementBox box = ui.get_element_box(kIds[i]);
    out.found[i] = box.found;
    out.width[i] = box.w;
    out.ink[i]   = box.found ? count_ink_rect(px, W, H, box.x, box.y, box.w, box.h) : 0;
  }
  return 0;
}

} // namespace

int main() {
  // EN: One hidden GLFW host window provides the GL context, REUSED by both sessions -- only
  //     RmlUi is re-initialised between them (never GLFW), so the two engines share an identical
  //     GL environment. A failed create() is a FAIL, not a SKIP (Xvfb provides a real, if
  //     software, GL context in CI) -- same convention as every other embed test in this suite.
  // PT: Uma única janela host GLFW oculta provê o contexto GL, REUSADA pelas duas sessões -- só o
  //     RmlUi é re-inicializado entre elas (nunca o GLFW), então os dois motores compartilham um
  //     ambiente GL idêntico. Um create() falho é FAIL, não SKIP (Xvfb provê um contexto GL real,
  //     ainda que por software, em CI) -- mesma convenção de todo outro teste embed desta suíte.
  glintfx::WindowGlfw host;
  if (!host.create("fonteng_ab_compare_host", W, H)) {
    std::puts("fonteng_ab_compare FAIL: host window create failed");
    return 1;
  }

  // EN: Session 1 -- glintfx's OWN engine (bypass=false).
  // PT: Sessão 1 -- motor PRÓPRIO do glintfx (bypass=false).
  EngineMetrics own;
  if (int rc = measure(false, "own", own)) return rc;

  // EN: Session 2 -- RmlUi's FreeType engine (bypass=true). The previous UiLayer is already
  //     destroyed here (own went out of measure()'s scope), so Rml::Shutdown() has reset the
  //     global font_interface to null and this session's Initialise() re-creates the FreeType
  //     default -- see this file's header for the confirmed RmlUi re-init semantics.
  // PT: Sessão 2 -- motor FreeType do RmlUi (bypass=true). A UiLayer anterior já está destruída
  //     aqui (own saiu do escopo de measure()), então o Rml::Shutdown() resetou o font_interface
  //     global pra nulo e o Initialise() desta sessão recria o FreeType default -- ver o cabeçalho
  //     deste arquivo pras semânticas de re-init confirmadas do RmlUi.
  EngineMetrics ft;
  if (int rc = measure(true, "freetype", ft)) return rc;

  // EN: Restore the default so no later test in a shared process (there is none today, but be
  //     hygienic) inherits a bypassed engine.
  // PT: Restaura o default para nenhum teste posterior num processo compartilhado (não há hoje,
  //     mas por higiene) herdar um motor bypassado.
  glintfx::own_font_engine_ab_bypass() = false;

  // EN: Tolerances -- see this file's header for the full justification.
  // PT: Tolerâncias -- ver o cabeçalho deste arquivo pra justificativa completa.
  constexpr float kWidthRelTol = 0.12f;   // 12%
  constexpr float kInkRatioLo  = 0.40f;
  constexpr float kInkRatioHi  = 2.50f;
  constexpr float kMinWidthPx  = 10.0f;   // EN: below this a % check is noise. PT: abaixo disto o % é ruído.
  constexpr long  kMinInk      = 20;      // EN: both engines must actually draw the span.

  std::printf("fonteng_ab_compare: %dx%d  span-by-span (own vs freetype)\n", W, H);
  std::printf("  %-13s %10s %10s %8s   %9s %9s %8s\n",
              "id", "w_own", "w_ft", "dW%", "ink_own", "ink_ft", "ink_r");

  int failures = 0;
  for (int i = 0; i < kN; ++i) {
    const float wo = own.width[i], wf = ft.width[i];
    const long  io = own.ink[i],   iff = ft.ink[i];
    const float dW = (wf > 0.f) ? std::fabs(wo - wf) / wf * 100.f : 0.f;
    const float inkR = (iff > 0) ? static_cast<float>(io) / static_cast<float>(iff) : 0.f;

    std::printf("  %-13s %10.2f %10.2f %7.1f%%   %9ld %9ld %8.2f\n",
                kIds[i], wo, wf, dW, io, iff, inkR);

    // EN: [0] both engines must have found + drawn the span (a not-found id or blank render is a
    //     fixture/engine failure, not a mere divergence).
    // PT: [0] os dois motores precisam ter achado + desenhado o span (id não-achado ou render em
    //     branco é falha de fixture/motor, não mera divergência).
    if (!own.found[i] || !ft.found[i]) {
      std::fprintf(stderr,
          "fonteng_ab_compare FAIL [0] '%s' get_element_box found own=%d ft=%d\n",
          kIds[i], own.found[i], ft.found[i]);
      ++failures;
      continue;
    }
    if (io < kMinInk || iff < kMinInk) {
      std::fprintf(stderr,
          "fonteng_ab_compare FAIL [1] '%s' rendered too little ink (own=%ld ft=%ld < %ld) -- "
          "a glyph (likely the pt-br composite accent) is not rasterising\n",
          kIds[i], io, iff, kMinInk);
      ++failures;
      continue;
    }

    // EN: [A] advance-width divergence -- catches a missing/short/2x-wrong advance.
    // PT: [A] divergência de largura de avanço -- pega um avanço faltando/curto/2x-errado.
    if (wf >= kMinWidthPx && dW > kWidthRelTol * 100.f) {
      std::fprintf(stderr,
          "fonteng_ab_compare FAIL [A] '%s' width diverges: own=%.2f ft=%.2f (%.1f%% > %.0f%%) -- "
          "own engine advance is grossly off FreeType's for the same glyphs\n",
          kIds[i], wo, wf, dW, kWidthRelTol * 100.f);
      ++failures;
    }

    // EN: [B] ink-coverage ratio -- catches a broken composite-accent outline whose advance is
    //     still correct (an accent that renders nothing, or a solid-fill blit bug).
    // PT: [B] ratio de cobertura de tinta -- pega um outline de acento composto quebrado cujo
    //     avanço ainda está certo (um acento que renderiza nada, ou um bug de blit sólido).
    if (inkR < kInkRatioLo || inkR > kInkRatioHi) {
      std::fprintf(stderr,
          "fonteng_ab_compare FAIL [B] '%s' ink ratio own/ft=%.2f outside [%.2f,%.2f] -- own "
          "engine glyph coverage diverges grossly from FreeType (broken accent outline?)\n",
          kIds[i], inkR, kInkRatioLo, kInkRatioHi);
      ++failures;
    }
  }

  if (failures == 0) {
    std::puts("fonteng_ab_compare: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_ab_compare: %d check(s) FAILED\n", failures);
  return failures;
}
