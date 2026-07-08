// SPDX-License-Identifier: MPL-2.0
// EN: Embed-path (glintfx::UiLayer) end-to-end proof for the data-model API
//     (AUD-TEC-7, 2026-07-08 audit finding). Closes a real coverage gap: the four
//     pre-existing data-model tests (data_model_smoke/scalar/list/rebind_uaf) all use
//     glintfx::App and are registered ONLY inside tests/CMakeLists.txt's
//     if(GLINTFX_BACKEND_GLFW) block -- the embed path (glintfx::UiLayer), which is what the
//     real consumer (GusWorld) actually uses, was NEVER exercised for create_data_model ->
//     bind_* -> load() -> set_*. This test registers OUTSIDE that block (like
//     click_callback_sanity/element_box_sanity/scroll_sanity) so it runs in BOTH build
//     configurations (GLFW=ON and embed-only GLFW=OFF).
//
//     Proof strategy: UiLayer has no App::snapshot() (no PPM/pixel readback), so this test
//     cannot reuse data_model_scalar.cpp's brightness-counting approach directly. Instead it
//     follows the SAME pattern already proven by click_callback_sanity/element_box_sanity/
//     scroll_sanity: observe state through UiLayer::get_element_box()'s PUBLIC geometry
//     return (found/x/y/w/h) against a purpose-built scene (data_model_embed_scene.rml/.rcss)
//     whose RCSS classes/data-if/data-for are wired so that each data-model mutation produces
//     an unambiguous, directly-assertable geometry delta -- no font, no @font-face, no pixel
//     comparison, so it is deterministic under Xvfb/Mesa-llvmpipe in EITHER build config (the
//     embed-only config never copies demos/showcase/assets/, so a font-dependent oracle like
//     scalar.rml/log.rml would silently render zero glyphs there).
//
//     Coverage this test closes (AUD-TEC-7):
//       (1) UiLayer::create_data_model/bind_number/bind_string/bind_bool/bind_list/load/
//           set_number/set_string/set_bool/set_list -- the entire lifecycle, via the embed
//           path, never exercised before this file.
//       (2) set_bool had ZERO tests anywhere in the suite before this file (grep-confirmed).
//           Covered here via data-if AND indirectly reflected through data-class-wide's
//           boolean expression on the number/string oracles.
//       (3) A second create_data_model() call must return false (DataBinder::impl_->created
//           limits the engine to ONE model; documented in app.hpp/ui_layer.hpp doc-comments
//           alongside this test).
//       (4) bind_* called AFTER load() must return false (view compilation already happened;
//           parity with the App-path proof in data_model_smoke.cpp, now proven on UiLayer too).
//
//     Each assertion pair below has "teeth": a directional round-trip (set true -> assert
//     changed, set back to the original value -> assert reverted) so that a no-op
//     implementation of any set_*/bind_* would fail at least one side of the pair -- not just
//     a single one-shot observation that a broken implementation could pass by accident (e.g.
//     an implementation that always returns the geometry for the LAST scene state, or one
//     that never actually writes through to the DataModel, would fail the "revert" half).
//
// PT: Prova ponta-a-ponta do caminho embed (glintfx::UiLayer) para a API de data-model
//     (achado de auditoria AUD-TEC-7, 2026-07-08). Fecha um gap de cobertura real: os quatro
//     testes pré-existentes de data-model (data_model_smoke/scalar/list/rebind_uaf) usam todos
//     glintfx::App e são registrados SÓ dentro do bloco if(GLINTFX_BACKEND_GLFW) do
//     tests/CMakeLists.txt -- o caminho embed (glintfx::UiLayer), que é o que o consumidor
//     real (GusWorld) de fato usa, NUNCA foi exercitado para create_data_model -> bind_* ->
//     load() -> set_*. Este teste é registrado FORA daquele bloco (como
//     click_callback_sanity/element_box_sanity/scroll_sanity) para rodar nas DUAS
//     configurações de build (GLFW=ON e embed-only GLFW=OFF).
//
//     Estratégia de prova: o UiLayer não tem App::snapshot() (sem readback de PPM/pixel),
//     então este teste não pode reusar diretamente a abordagem de contagem de brilho do
//     data_model_scalar.cpp. Em vez disso segue o MESMO padrão já provado por
//     click_callback_sanity/element_box_sanity/scroll_sanity: observar estado através do
//     retorno PÚBLICO de geometria de UiLayer::get_element_box() (found/x/y/w/h) contra uma
//     cena feita sob medida (data_model_embed_scene.rml/.rcss) cujas classes RCSS/data-if/
//     data-for são conectadas de forma que cada mutação do data-model produz um delta de
//     geometria inequívoco e diretamente verificável -- sem fonte, sem @font-face, sem
//     comparação de pixel, então é determinístico sob Xvfb/Mesa-llvmpipe em QUALQUER config de
//     build (a config embed-only nunca copia demos/showcase/assets/, então um oráculo
//     dependente de fonte como scalar.rml/log.rml renderizaria zero glifos silenciosamente lá).
//
//     Cobertura que este teste fecha (AUD-TEC-7):
//       (1) UiLayer::create_data_model/bind_number/bind_string/bind_bool/bind_list/load/
//           set_number/set_string/set_bool/set_list -- o ciclo de vida inteiro, via caminho
//           embed, nunca exercitado antes deste arquivo.
//       (2) set_bool tinha ZERO testes em qualquer lugar da suíte antes deste arquivo
//           (confirmado por grep). Coberto aqui via data-if E refletido indiretamente pela
//           expressão booleana de data-class-wide nos oráculos number/string.
//       (3) Uma segunda chamada a create_data_model() deve retornar false
//           (DataBinder::impl_->created limita o engine a UM modelo; documentado nos
//           doc-comments de app.hpp/ui_layer.hpp junto com este teste).
//       (4) bind_* chamado APÓS load() deve retornar false (compilação de views já aconteceu;
//           paridade com a prova do caminho App em data_model_smoke.cpp, agora provada também
//           no UiLayer).
//
//     Cada par de asserção abaixo tem "dente": um round-trip direcional (muda para true ->
//     verifica mudança, muda de volta pro valor original -> verifica reversão) para que uma
//     implementação no-op de qualquer set_*/bind_* falhe pelo menos um lado do par -- não
//     apenas uma observação de tiro único que uma implementação quebrada poderia passar por
//     acidente (ex.: uma implementação que sempre retorna a geometria do ÚLTIMO estado da
//     cena, ou uma que nunca escreve de fato no DataModel, falharia a metade "reverte").
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cmath>

static bool approx(float a, float b, float tol = 0.5f) { return std::fabs(a - b) <= tol; }

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("data_model_embed_host", 200, 150)) { std::puts("FAIL: host create"); return 1; }

  glintfx::UiLayer ui({ .logical_width = 200, .logical_height = 150, .load_gl = true });
  if (!ui.ok()) { std::puts("FAIL: ui attach"); return 2; }

  // ---------------------------------------------------------------------------
  // Setup: create_data_model -> bind_number/bind_string/bind_bool/bind_list -> load.
  //        All bind_* MUST happen before load() so their data-views compile at parse time
  //        (RmlUi constraint, same as every other data-model test in this suite).
  // ---------------------------------------------------------------------------
  const bool ok_model = ui.create_data_model("hud");
  const bool ok_hp     = ui.bind_number("hp", 0.0);
  const bool ok_label  = ui.bind_string("label", "a");
  const bool ok_flag   = ui.bind_bool("flag", false);
  const bool ok_log    = ui.bind_list("log");

  // (3, AUD-TEC-7) A second create_data_model() call -- even with a DIFFERENT name -- must
  // return false: DataBinder allows at most ONE model per engine instance (impl_->created
  // guard, check-before-mutate). This was previously untested and undocumented.
  const bool ok_second_model = ui.create_data_model("hud2");

  const bool ok_load = ui.load("data_model_embed_scene.rml");

  std::printf("create_data_model(hud):   %s (expect OK)\n",    ok_model  ? "OK" : "FAIL");
  std::printf("bind_number(hp):          %s (expect OK)\n",    ok_hp     ? "OK" : "FAIL");
  std::printf("bind_string(label):       %s (expect OK)\n",    ok_label  ? "OK" : "FAIL");
  std::printf("bind_bool(flag):          %s (expect OK)\n",    ok_flag   ? "OK" : "FAIL");
  std::printf("bind_list(log):           %s (expect OK)\n",    ok_log    ? "OK" : "FAIL");
  std::printf("create_data_model(hud2):  %s (expect false, only one model allowed)\n",
              !ok_second_model ? "OK" : "FAIL");
  std::printf("load(scene):              %s (expect OK)\n",    ok_load   ? "OK" : "FAIL");

  bool all_ok = ok_model && ok_hp && ok_label && ok_flag && ok_log && !ok_second_model && ok_load;
  if (!ok_load) { std::puts("data_model_embed_sanity FAIL: setup"); return 3; }

  // (4) bind_* AFTER load() must return false (views already compiled) -- parity with the
  //     App-path proof in data_model_smoke.cpp, now proven on UiLayer too.
  const bool late_bind = ui.bind_number("too_late", 1.0);
  std::printf("late bind_number:         %s (expect false)\n", !late_bind ? "OK" : "FAIL");
  all_ok = all_ok && !late_bind;

  ui.update(); ui.render();

  // ---------------------------------------------------------------------------
  // Baseline (hp=0, label="a", flag=false, log=[]):
  //   #hpbox    -- "hp > 50" is false  -> no ".wide" class -> w ~20 (base RCSS width).
  //   #namebox  -- "label == 'B'" is false -> no ".wide" class -> w ~20.
  //   #flagpanel-- data-if="flag" is false -> RmlUi sets display:none on the element itself
  //                (DataViewIf::Update, confirmed by reading DataViewDefault.cpp) -- it stays
  //                in the DOM (get_element_box still finds it) but collapses to a 0x0 box.
  //   item-0    -- log is empty -> no data-for clone exists yet -> not found.
  // ---------------------------------------------------------------------------
  auto hpbox_0 = ui.get_element_box("hpbox");
  if (!hpbox_0.found || !approx(hpbox_0.w, 20.f)) {
    std::fprintf(stderr, "FAIL: hpbox baseline found=%d w=%.1f (expected found=1 w~20)\n",
                 hpbox_0.found, hpbox_0.w);
    return 4;
  }
  auto namebox_0 = ui.get_element_box("namebox");
  if (!namebox_0.found || !approx(namebox_0.w, 20.f)) {
    std::fprintf(stderr, "FAIL: namebox baseline found=%d w=%.1f (expected found=1 w~20)\n",
                 namebox_0.found, namebox_0.w);
    return 5;
  }
  auto flagpanel_0 = ui.get_element_box("flagpanel");
  if (!flagpanel_0.found || !(flagpanel_0.w < 1.f && flagpanel_0.h < 1.f)) {
    std::fprintf(stderr,
                 "FAIL: flagpanel baseline found=%d w=%.1f h=%.1f "
                 "(expected found=1, collapsed 0x0 box under data-if=false)\n",
                 flagpanel_0.found, flagpanel_0.w, flagpanel_0.h);
    return 6;
  }
  auto flagwide_0 = ui.get_element_box("flagwide");
  if (!flagwide_0.found || !approx(flagwide_0.w, 20.f)) {
    std::fprintf(stderr, "FAIL: flagwide baseline found=%d w=%.1f (expected found=1 w~20)\n",
                 flagwide_0.found, flagwide_0.w);
    return 100;
  }
  auto item0_0 = ui.get_element_box("item-0");
  if (item0_0.found) { std::puts("FAIL: item-0 found before any set_list (log is empty)"); return 7; }

  std::puts("data_model_embed_sanity: baseline OK");

  // ---------------------------------------------------------------------------
  // (1a, number) set_number("hp", 99.0) -> hp > 50 becomes true -> #hpbox gains ".wide" -> w~80.
  //              Dente: revert to 10.0 (still <= 50) -> #hpbox loses ".wide" -> w~20 again.
  // ---------------------------------------------------------------------------
  ui.set_number("hp", 99.0);
  ui.update(); ui.render();
  auto hpbox_hi = ui.get_element_box("hpbox");
  if (!hpbox_hi.found || !approx(hpbox_hi.w, 80.f)) {
    std::fprintf(stderr, "FAIL: hpbox after set_number(99) w=%.1f (expected ~80)\n", hpbox_hi.w);
    return 8;
  }

  ui.set_number("hp", 10.0);
  ui.update(); ui.render();
  auto hpbox_lo = ui.get_element_box("hpbox");
  if (!hpbox_lo.found || !approx(hpbox_lo.w, 20.f)) {
    std::fprintf(stderr, "FAIL: hpbox after set_number(10) w=%.1f (expected ~20, reverted)\n",
                 hpbox_lo.w);
    return 9;
  }
  std::puts("data_model_embed_sanity: set_number round-trip OK");

  // ---------------------------------------------------------------------------
  // (1b, string) set_string("label", "B") -> label == 'B' becomes true -> #namebox w~80.
  //              Dente: revert to "a" -> w~20 again.
  // ---------------------------------------------------------------------------
  ui.set_string("label", "B");
  ui.update(); ui.render();
  auto namebox_b = ui.get_element_box("namebox");
  if (!namebox_b.found || !approx(namebox_b.w, 80.f)) {
    std::fprintf(stderr, "FAIL: namebox after set_string(\"B\") w=%.1f (expected ~80)\n",
                 namebox_b.w);
    return 10;
  }

  ui.set_string("label", "a");
  ui.update(); ui.render();
  auto namebox_a = ui.get_element_box("namebox");
  if (!namebox_a.found || !approx(namebox_a.w, 20.f)) {
    std::fprintf(stderr, "FAIL: namebox after set_string(\"a\") w=%.1f (expected ~20, reverted)\n",
                 namebox_a.w);
    return 11;
  }
  std::puts("data_model_embed_sanity: set_string round-trip OK");

  // ---------------------------------------------------------------------------
  // (1c/2, bool -- set_bool had ZERO tests before this file) TWO independent oracles, both
  //   driven by the SAME "flag" variable, needed because of a genuine RmlUi layout-caching
  //   characteristic confirmed by instrumenting DataViewDefault.cpp directly during this
  //   test's development (reproduces even with a plain in-flow, non-absolute element -- not a
  //   glintfx bug): a data-if element's cached border-box (Element::GetBox()) collapses to 0x0
  //   reliably ONLY the first time it goes visible->invisible from a NEVER-laid-out baseline;
  //   once laid out with a real box, a LATER display:none does NOT reset the cached box back to
  //   zero, even though DataViewIf::Update *does* correctly toggle the Display property every
  //   time (verified: the property flips correctly, only the stale geometry readback does not).
  //
  //   (A) #flagpanel / data-if -- the ONE reliable direction: never-visible -> visible.
  //       set_bool("flag", true) makes data-if="flag" true -> display:none is removed for the
  //       FIRST time -> box becomes the RCSS-authored 50x50. Real teeth: if set_bool were a
  //       no-op, this box would stay 0x0.
  //   (B) #flagwide / data-class-wide -- the ROUND-TRIP-SAFE oracle (same mechanism already
  //       proven reversible above for hp/label): set_bool("flag", true) -> ".wide" applied ->
  //       w~80; set_bool("flag", false) -> ".wide" removed -> w~20 again. This is what actually
  //       proves set_bool propagates BOTH directions, not just the one-shot reveal in (A).
  // ---------------------------------------------------------------------------
  ui.set_bool("flag", true);
  ui.update(); ui.render();
  auto flagpanel_on = ui.get_element_box("flagpanel");
  if (!flagpanel_on.found || !approx(flagpanel_on.w, 50.f) || !approx(flagpanel_on.h, 50.f)) {
    std::fprintf(stderr,
                 "FAIL: flagpanel after set_bool(true) found=%d w=%.1f h=%.1f "
                 "(expected found=1 ~50x50)\n",
                 flagpanel_on.found, flagpanel_on.w, flagpanel_on.h);
    return 12;
  }
  auto flagwide_on = ui.get_element_box("flagwide");
  if (!flagwide_on.found || !approx(flagwide_on.w, 80.f)) {
    std::fprintf(stderr, "FAIL: flagwide after set_bool(true) w=%.1f (expected ~80)\n",
                 flagwide_on.w);
    return 101;
  }

  ui.set_bool("flag", false);
  ui.update(); ui.render();
  auto flagwide_off = ui.get_element_box("flagwide");
  if (!flagwide_off.found || !approx(flagwide_off.w, 20.f)) {
    std::fprintf(stderr,
                 "FAIL: flagwide after set_bool(false) w=%.1f (expected ~20, reverted)\n",
                 flagwide_off.w);
    return 102;
  }
  std::puts("data_model_embed_sanity: set_bool round-trip OK (data-if reveal + data-class round-trip)");

  // ---------------------------------------------------------------------------
  // (1d, list) set_list("log", {"x","y","z"}, 3) -> data-for clones three <div class="item">
  //   rows, tagged item-0/item-1/item-2 via data-attr-id="'item-' + it_index". item-3 must NOT
  //   exist (only 3 entries). Dente: shrink to 1 item -> item-1 (which existed a moment ago)
  //   must be torn down by DataViewFor's shrink path (RemoveChild), no longer found.
  // ---------------------------------------------------------------------------
  const char* items3[] = {"x", "y", "z"};
  ui.set_list("log", items3, 3);
  ui.update(); ui.render();
  auto item0_3 = ui.get_element_box("item-0");
  auto item1_3 = ui.get_element_box("item-1");
  auto item2_3 = ui.get_element_box("item-2");
  auto item3_3 = ui.get_element_box("item-3");
  if (!item0_3.found || !item1_3.found || !item2_3.found) {
    std::fprintf(stderr,
                 "FAIL: after set_list(x,y,z): item-0=%d item-1=%d item-2=%d (expected all found)\n",
                 item0_3.found, item1_3.found, item2_3.found);
    return 14;
  }
  if (item3_3.found) { std::puts("FAIL: item-3 found after set_list with only 3 entries"); return 15; }

  const char* items1[] = {"solo"};
  ui.set_list("log", items1, 1);
  ui.update(); ui.render();
  auto item0_1 = ui.get_element_box("item-0");
  auto item1_1 = ui.get_element_box("item-1");
  if (!item0_1.found) { std::puts("FAIL: item-0 not found after shrinking set_list to 1 entry"); return 16; }
  if (item1_1.found) {
    std::puts("FAIL: item-1 still found after shrinking set_list to 1 entry "
              "(data-for did not tear down the extra clone)");
    return 17;
  }
  std::puts("data_model_embed_sanity: set_list round-trip OK (grow to 3, shrink to 1)");

  if (!ui.ok()) { std::puts("FAIL: ok() false after full data-model sequence"); return 18; }
  if (!all_ok) { std::puts("data_model_embed_sanity FAIL: setup-phase assertions"); return 19; }

  std::puts("data_model_embed_sanity: PASS");
  return 0;
}
