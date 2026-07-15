// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_engine_select_sanity -- L1.20-FONTFLIP Phase 2/3 (FT-F4) gate: proves the PUBLIC
//     `UiLayerConfig::font_engine` field (glintfx/include/glintfx/font_engine.hpp) actually
//     selects and drives the engine, end-to-end -- through the field a real consumer sets, NOT
//     through the src-internal `own_font_engine_ab_bypass()` test hook every other fonteng_ab_*
//     test in this suite uses. Before this file, NOTHING in the tracked suite exercised
//     `FontEngine::FreeType` via the public API at all: every existing embed test either leaves
//     `UiLayerConfig` at its default (`font_engine = FontEngine::Own`) or drives the engine via
//     the internal bypass. A regression that silently ignored `cfg.font_engine` and always
//     installed the own engine regardless of the caller's request would have gone completely
//     undetected by the rest of the suite.
//
//     TWO SESSIONS, PUBLIC FIELD ONLY. Constructs `glintfx::UiLayer` twice, sequentially, in one
//     process: once with `UiLayerConfig{.font_engine = FontEngine::Own}`, once with
//     `UiLayerConfig{.font_engine = FontEngine::FreeType}` -- reusing the SAME
//     tests/fonteng_ab_scene.rml document and the SAME hidden-GLFW-host-window GL-context
//     fixture pattern fonteng_ab_compare.cpp already validated (RmlUi's Shutdown() between
//     sessions resets the global font_interface to null, so each UiLayer ctor's Bootstrap::init()
//     independently re-resolves the engine from ITS OWN `cfg.font_engine`, with no
//     cross-contamination -- see fonteng_ab_compare.cpp's header for the confirmed RmlUi
//     re-init semantics this relies on). `own_font_engine_ab_bypass()` is never touched here.
//
//     TWO PROOFS, both from the SAME two-session run:
//       (a) ROLLBACK WORKS -- both configs must produce a valid render: UiLayer::ok() true,
//           every span found via get_element_box(), every span's advance width AND ink coverage
//           strictly positive (above the same kMinInk/kMinWidthPx floors fonteng_ab_compare.cpp
//           uses). This is the load-bearing proof for a consumer that hits a rendering
//           regression with the own engine and flips `font_engine = FontEngine::FreeType` as a
//           one-line, no-rebuild rollback (per FontEngine's own doc-comment contract): if
//           FreeType-via-the-public-field silently produced a blank/zero-width render, the
//           documented rollback path would be a lie.
//       (b) THE FIELD IS NOT A NO-OP -- at least one span's metrics (width or ink) must diverge
//           between the two sessions by more than pure floating-point/AA noise. If a bug made
//           `cfg.font_engine` dead code (e.g. Bootstrap::init() ignoring the parameter and
//           always installing the own engine, or always falling through to FreeType regardless
//           of the request), BOTH sessions would render through the SAME actual engine and this
//           two-session run would produce numerically-identical metrics -- (a) alone would still
//           pass (both sessions still render something valid), which is exactly why (a) alone
//           cannot catch a no-op field: (b) is the check that specifically catches that failure
//           mode. The tolerance is a small ABSOLUTE floor per span (kNoopWidthEpsPx=0.05px,
//           kNoopInkEpsFrac=1% of the larger ink count) -- deliberately far tighter than
//           fonteng_ab_compare.cpp's 12%/[0.40,2.5] GROSS-divergence bands (this check is not
//           trying to catch a broken glyph, it is trying to catch two renders of the exact same
//           bytes through the exact same rasteriser, which fonteng_ab_compare.cpp's own probe
//           data confirms do not coincidentally land on bit-identical floats -- see that file's
//           header for the measured own-vs-FreeType deltas on this very scene, all comfortably
//           above these no-op floors). Any SINGLE span clearing the floor is sufficient (the
//           check is "the two engines are observably different", not "every span differs").
//
//     Deterministic (no flakiness): identical basis to fonteng_ab_compare.cpp -- pure layout math
//     for width, stable ink counts under llvmpipe for plain un-blurred text. No sleeps, no
//     retries.
//
//     Registered in tests/CMakeLists.txt's `if(GLINTFX_OWN_FONT_ENGINE)` block, OUTSIDE
//     `if(GLINTFX_BACKEND_GLFW)`, via the `${_embed_dep}` target -- same pattern as
//     fonteng_ab_compare.cpp right above it -- so this ONE file builds and runs identically with
//     `GLINTFX_BACKEND_GLFW` ON or OFF, covering all 4 corners of {GLFW ON/OFF} x {own-default,
//     FreeType-via-config} across the two build configs.
// PT: fonteng_engine_select_sanity -- gate do L1.20-FONTFLIP Fase 2/3 (FT-F4): prova que o campo
//     PÚBLICO `UiLayerConfig::font_engine` (glintfx/include/glintfx/font_engine.hpp) de fato
//     seleciona e dirige o motor, ponta-a-ponta -- pelo campo que um consumidor real define, NÃO
//     pelo hook src-interno só-de-teste `own_font_engine_ab_bypass()` que todo outro teste
//     fonteng_ab_* desta suíte usa. Antes deste arquivo, NADA na suíte rastreada exercitava
//     `FontEngine::FreeType` pela API pública: todo teste embed existente ou deixa o
//     `UiLayerConfig` no default (`font_engine = FontEngine::Own`) ou dirige o motor pelo bypass
//     interno. Uma regressão que ignorasse silenciosamente `cfg.font_engine` e sempre instalasse
//     o motor próprio, independente do pedido do chamador, passaria completamente despercebida
//     pelo resto da suíte.
//
//     DUAS SESSÕES, SÓ CAMPO PÚBLICO. Constrói `glintfx::UiLayer` duas vezes, sequencialmente,
//     num processo: uma com `UiLayerConfig{.font_engine = FontEngine::Own}`, uma com
//     `UiLayerConfig{.font_engine = FontEngine::FreeType}` -- reusando o MESMO documento
//     tests/fonteng_ab_scene.rml e o MESMO padrão de fixture de contexto GL de janela-host-GLFW-
//     oculta que fonteng_ab_compare.cpp já validou (o Shutdown() do RmlUi entre sessões reseta o
//     font_interface global pra nulo, então o Bootstrap::init() do ctor de cada UiLayer
//     re-resolve independentemente o motor a partir do SEU PRÓPRIO `cfg.font_engine`, sem
//     contaminação cruzada -- ver o cabeçalho de fonteng_ab_compare.cpp pras semânticas
//     confirmadas de re-init do RmlUi das quais isto depende). `own_font_engine_ab_bypass()`
//     nunca é tocado aqui.
//
//     DUAS PROVAS, ambas da MESMA execução de duas sessões:
//       (a) O ROLLBACK FUNCIONA -- os dois configs precisam produzir um render válido:
//           UiLayer::ok() true, todo span achado via get_element_box(), a largura de avanço E a
//           cobertura de tinta de todo span estritamente positivas (acima dos mesmos pisos
//           kMinInk/kMinWidthPx que fonteng_ab_compare.cpp usa). Esta é a prova de sustentação
//           para um consumidor que encontra uma regressão de render com o motor próprio e vira
//           `font_engine = FontEngine::FreeType` como rollback de uma linha, sem rebuild
//           (conforme o contrato do próprio doc-comment de FontEngine): se o FreeType-via-campo-
//           público renderizasse silenciosamente em branco/largura-zero, o caminho de rollback
//           documentado seria mentira.
//       (b) O CAMPO NÃO É NO-OP -- pelo menos um span precisa divergir em métrica (largura ou
//           tinta) entre as duas sessões além de puro ruído de ponto flutuante/AA. Se um bug
//           tornasse `cfg.font_engine` código morto (ex.: Bootstrap::init() ignorando o parâmetro
//           e sempre instalando o motor próprio, ou sempre caindo no FreeType independente do
//           pedido), AS DUAS sessões renderizariam pelo MESMO motor de fato e esta execução de
//           duas sessões produziria métricas numericamente IDÊNTICAS -- (a) sozinho ainda
//           passaria (as duas sessões ainda renderizam algo válido), exatamente por isso (a)
//           sozinho não consegue pegar um campo no-op: (b) é o cheque que especificamente pega
//           esse modo de falha. A tolerância é um piso ABSOLUTO pequeno por span
//           (kNoopWidthEpsPx=0.05px, kNoopInkEpsFrac=1% da maior contagem de tinta) --
//           deliberadamente bem mais apertada que as bandas de divergência GROSSEIRA de 12%/
//           [0.40,2.5] do fonteng_ab_compare.cpp (este cheque não tenta pegar um glyph quebrado,
//           tenta pegar dois renders dos MESMOS bytes através do MESMO rasterizador, que os
//           próprios dados de sonda do fonteng_ab_compare.cpp confirmam não caírem
//           coincidentemente em floats bit-idênticos -- ver o cabeçalho daquele arquivo pros
//           deltas medidos próprio-vs-FreeType nesta mesma cena, todos confortavelmente acima
//           destes pisos de no-op). Qualquer ÚNICO span que ultrapasse o piso já basta (o cheque
//           é "os dois motores são observavelmente diferentes", não "todo span diverge").
//
//     Determinístico (sem flakiness): mesma base de fonteng_ab_compare.cpp -- pura matemática de
//     layout para largura, contagens de tinta estáveis sob llvmpipe para texto simples sem blur.
//     Sem sleeps, sem retries.
//
//     Registrado no bloco `if(GLINTFX_OWN_FONT_ENGINE)` do tests/CMakeLists.txt, FORA de
//     `if(GLINTFX_BACKEND_GLFW)`, via o alvo `${_embed_dep}` -- mesmo padrão de
//     fonteng_ab_compare.cpp logo acima -- então este ÚNICO arquivo builda e roda identicamente
//     com `GLINTFX_BACKEND_GLFW` ON ou OFF, cobrindo os 4 cantos de {GLFW ON/OFF} x
//     {próprio-default, FreeType-via-config} através das duas configs de build.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

constexpr int W = 640, H = 360;

// EN: Same spans as fonteng_ab_scene.rml / fonteng_ab_compare.cpp, in document order.
// PT: Mesmos spans de fonteng_ab_scene.rml / fonteng_ab_compare.cpp, na ordem do documento.
const char* const kIds[] = {
    "ascii_caps", "ascii_low", "accent_low", "accent_up", "word_coracao", "word_acao",
};
constexpr int kN = static_cast<int>(sizeof(kIds) / sizeof(kIds[0]));

struct EngineMetrics {
  bool  found[kN] = {};
  float width[kN] = {};
  long  ink[kN]   = {};
};

// EN: See fonteng_ab_compare.cpp's identical helper for the flip rationale (glReadPixels is
//     bottom-left origin; get_element_box() is top-left, y-down).
// PT: Ver o helper idêntico de fonteng_ab_compare.cpp pro racional do flip (origem do
//     glReadPixels é bottom-left; get_element_box() é top-left, y-para-baixo).
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

// EN: Runs ONE full UiLayer session selecting the engine SOLELY via the public
//     `UiLayerConfig::font_engine` field passed in `engine` -- no bypass, no A/B hook. Returns 0
//     on success, non-zero on a fixture failure.
// PT: Roda UMA sessão UiLayer completa selecionando o motor SÓ pelo campo público
//     `UiLayerConfig::font_engine` passado em `engine` -- sem bypass, sem hook A/B. Retorna 0 no
//     sucesso, não-zero numa falha de fixture.
int measure(glintfx::FontEngine engine, const char* label, EngineMetrics& out) {
  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true,
                         .font_engine = engine });
  if (!ui.ok()) {
    std::fprintf(stderr, "fonteng_engine_select_sanity FAIL: UiLayer attach failed (%s)\n",
                 label);
    return 10;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_ab_scene.rml")) {
    std::fprintf(stderr, "fonteng_engine_select_sanity FAIL: load() returned false (%s)\n",
                 label);
    return 11;
  }

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
  glintfx::WindowGlfw host;
  if (!host.create("fonteng_engine_select_sanity_host", W, H)) {
    std::puts("fonteng_engine_select_sanity FAIL: host window create failed");
    return 1;
  }

  // EN: Session 1 -- FontEngine::Own via the PUBLIC field.
  // PT: Sessão 1 -- FontEngine::Own pelo campo PÚBLICO.
  EngineMetrics own;
  if (int rc = measure(glintfx::FontEngine::Own, "own(public-field)", own)) return rc;

  // EN: Session 2 -- FontEngine::FreeType via the PUBLIC field. RmlUi's Shutdown() (called when
  //     `own` above went out of measure()'s scope) reset the global font_interface, so this
  //     session's Bootstrap::init() re-resolves purely from ITS OWN `cfg.font_engine` -- see this
  //     file's header for the confirmed re-init semantics.
  // PT: Sessão 2 -- FontEngine::FreeType pelo campo PÚBLICO. O Shutdown() do RmlUi (chamado
  //     quando `own` acima saiu do escopo de measure()) resetou o font_interface global, então o
  //     Bootstrap::init() desta sessão re-resolve puramente do SEU PRÓPRIO `cfg.font_engine` --
  //     ver o cabeçalho deste arquivo pras semânticas de re-init confirmadas.
  EngineMetrics ft;
  if (int rc = measure(glintfx::FontEngine::FreeType, "freetype(public-field)", ft)) return rc;

  constexpr float kMinWidthPx     = 10.0f;
  constexpr long  kMinInk         = 20;
  constexpr float kNoopWidthEpsPx = 0.05f;  // EN: 20x tighter than a sub-pixel rounding artefact.
                                             // PT: 20x mais apertado que um artefato de
                                             // arredondamento sub-pixel.
  constexpr float kNoopInkEpsFrac = 0.01f;  // 1%

  std::printf("fonteng_engine_select_sanity: %dx%d  span-by-span "
              "(FontEngine::Own vs FontEngine::FreeType, both via UiLayerConfig::font_engine)\n",
              W, H);
  std::printf("  %-13s %10s %10s %8s   %9s %9s\n",
              "id", "w_own", "w_ft", "dW(px)", "ink_own", "ink_ft");

  int failures = 0;
  bool any_engine_diff = false;

  for (int i = 0; i < kN; ++i) {
    const float wo = own.width[i], wf = ft.width[i];
    const long  io = own.ink[i],   iff = ft.ink[i];
    const float dW = std::fabs(wo - wf);

    std::printf("  %-13s %10.3f %10.3f %8.3f   %9ld %9ld\n", kIds[i], wo, wf, dW, io, iff);

    // EN: (a) ROLLBACK WORKS -- both public-field selections must actually render.
    // PT: (a) O ROLLBACK FUNCIONA -- as duas seleções via campo público precisam de fato
    //     renderizar.
    if (!own.found[i] || !ft.found[i]) {
      std::fprintf(stderr,
          "fonteng_engine_select_sanity FAIL [a-found] '%s' get_element_box found own=%d ft=%d\n",
          kIds[i], own.found[i], ft.found[i]);
      ++failures;
      continue;
    }
    if (wo < kMinWidthPx || wf < kMinWidthPx) {
      std::fprintf(stderr,
          "fonteng_engine_select_sanity FAIL [a-width] '%s' width too small (own=%.3f ft=%.3f "
          "< %.1fpx) -- a public-field selection failed to actually render this span\n",
          kIds[i], wo, wf, kMinWidthPx);
      ++failures;
    }
    if (io < kMinInk || iff < kMinInk) {
      std::fprintf(stderr,
          "fonteng_engine_select_sanity FAIL [a-ink] '%s' ink too low (own=%ld ft=%ld < %ld) -- "
          "a public-field selection rendered blank/near-blank\n",
          kIds[i], io, iff, kMinInk);
      ++failures;
    }

    // EN: (b) THE FIELD IS NOT A NO-OP -- track whether ANY span cleared the tight no-op floor.
    // PT: (b) O CAMPO NÃO É NO-OP -- rastreia se ALGUM span ultrapassou o piso apertado de no-op.
    const float ink_denom = static_cast<float>(std::max(io, iff));
    const float ink_frac  = (ink_denom > 0.f)
        ? static_cast<float>(std::labs(io - iff)) / ink_denom : 0.f;
    if (dW > kNoopWidthEpsPx || ink_frac > kNoopInkEpsFrac) any_engine_diff = true;
  }

  if (!any_engine_diff) {
    std::fprintf(stderr,
        "fonteng_engine_select_sanity FAIL [b-noop] every span's width/ink is numerically "
        "identical (within %.2fpx / %.0f%%) between FontEngine::Own and FontEngine::FreeType -- "
        "UiLayerConfig::font_engine looks like DEAD CODE: both sessions rendered through the "
        "SAME actual engine\n", kNoopWidthEpsPx, kNoopInkEpsFrac * 100.f);
    ++failures;
  }

  if (failures == 0) {
    std::puts("fonteng_engine_select_sanity: PASS "
               "(rollback renders + public field is not a no-op)");
    return 0;
  }
  std::fprintf(stderr, "fonteng_engine_select_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
