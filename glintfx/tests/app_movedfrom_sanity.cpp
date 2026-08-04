// SPDX-License-Identifier: Apache-2.0
// EN: AUD-APP-MOVEDFROM -- ENUMERATES every public App method (not a sample) and calls each one
//     on a MOVED-FROM App (`impl_ == nullptr`), asserting fail-high (the exact value each method
//     already documents/returns for `!ok()`), never a crash. Before the fix this file exists to
//     drive, the guard pattern throughout app.cpp is `if (!impl_->ok) ...`, which dereferences
//     `impl_` BEFORE checking whether it is null -- confirmed live (segfault) by the W18
//     adversarial reviewer's probe. The RED this test produces pre-fix is exactly that segfault:
//     ctest reports a crashed process, not a clean assertion failure -- expected, and the proof
//     the guard bites (house rule 2026-07-27: "enumeration finds what you didn't know to
//     suspect" -- a sampled subset of methods could pass while the un-sampled ones still
//     segfault).
//
//     METHOD COUNT (paridade com o header): kExpectedPublicMethods below is the hand-verified
//     count of glintfx::App's public member functions in glintfx/include/glintfx/app.hpp,
//     EXCLUDING the four special members (constructor, destructor, move constructor, move
//     assignment -- inherently safe on a moved-from object already: the dtor is a no-op on a
//     null impl_, and the move ops accept a moved-from source by construction). ok() IS
//     included (it is a public method, and it is the one site the class already made
//     null-safe -- exercising it here proves the enumeration is complete, not merely "every
//     method except the one that already worked"). Counted 2026-08-04 against app.hpp: 63.
//     If a future App method is added/removed, this constant AND the calls below must be
//     updated together -- a mismatch fails loudly (see main()'s final check) instead of
//     silently under-covering the surface.
// PT: AUD-APP-MOVEDFROM -- ENUMERA todo método público de App (não uma amostra) e chama cada um
//     num App MOVIDO-DE (`impl_ == nullptr`), verificando fail-high (o valor exato que cada
//     método já documenta/devolve para `!ok()`), nunca um crash. Antes do fix que este arquivo
//     existe para dirigir, o padrão de guarda por todo app.cpp é `if (!impl_->ok) ...`, que
//     derreferencia `impl_` ANTES de checar se é nulo -- confirmado ao vivo (segfault) pela
//     sonda do reviewer adversarial da W18. O VERMELHO que este teste produz pré-fix é
//     exatamente esse segfault: o ctest reporta um processo que crashou, não uma falha de
//     assert limpa -- esperado, e a prova de que a guarda morde (regra da casa 2026-07-27:
//     "enumeração encontra o que você não sabia que devia suspeitar" -- uma amostra de métodos
//     podia passar enquanto os não-amostrados ainda segfaltavam).
//
//     CONTAGEM DE MÉTODOS (paridade com o header): kExpectedPublicMethods abaixo é a contagem
//     verificada à mão dos métodos-membro públicos de glintfx::App em
//     glintfx/include/glintfx/app.hpp, EXCLUINDO os quatro membros especiais (construtor,
//     destrutor, construtor de move, atribuição de move -- já inerentemente seguros num objeto
//     movido-de: o destrutor é no-op sobre um impl_ nulo, e as operações de move aceitam uma
//     origem movida-de por construção). ok() ESTÁ incluído (é um método público, e é o único
//     sítio que a classe já tornava null-safe -- exercitá-lo aqui prova que a enumeração está
//     completa, não apenas "todo método exceto o que já funcionava"). Contado em 2026-08-04
//     contra app.hpp: 63. Se um método futuro de App for adicionado/removido, esta constante E
//     as chamadas abaixo precisam ser atualizadas juntas -- uma divergência falha ruidosamente
//     (ver o cheque final do main()) em vez de sub-cobrir a superfície silenciosamente.
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstring>
#include <utility>

using namespace glintfx;

namespace {

constexpr int kExpectedPublicMethods = 63;

int g_exercised = 0;
int g_failures = 0;

void expect(const char* method, bool condition) {
  ++g_exercised;
  if (!condition) {
    ++g_failures;
    std::fprintf(stderr, "app_movedfrom_sanity FAIL: %s did not fail-high on a moved-from App\n",
                 method);
  }
}

} // namespace

int main() {
  AppConfig cfg;
  cfg.title = "app_movedfrom_sanity";
  cfg.width = 320;
  cfg.height = 240;
  App a(cfg);
  if (!a.ok()) {
    // EN: Setup failure (e.g. no GL context under this environment) is a test-infra problem,
    //     not the bug under test -- report it distinctly from a moved-from assertion failure.
    // PT: Falha de setup (ex.: sem contexto GL neste ambiente) é problema de infra de teste,
    //     não o bug sob teste -- reportar distinto de uma falha de asserção movido-de.
    std::fprintf(stderr,
                 "app_movedfrom_sanity SETUP FAIL: App construction failed (ok()==false "
                 "before any move)\n");
    return 1;
  }

  // EN: The move that puts `a` in the moved-from state under test. `b` stays alive (and owns
  //     the real window/GL context/UI engine) for the whole test so the process has a valid App
  //     to destruct cleanly at scope exit, same as every other App test in this suite.
  // PT: O move que coloca `a` no estado movido-de sob teste. `b` fica viva (e é dona da
  //     janela/contexto GL/motor de UI reais) pelo teste inteiro, para o processo ter um App
  //     válido para destruir de forma limpa na saída de escopo, igual a todo outro teste de App
  //     desta suíte.
  App b(std::move(a));
  if (!b.ok()) {
    std::fprintf(stderr, "app_movedfrom_sanity SETUP FAIL: move target b.ok()==false\n");
    return 1;
  }

  // ---------------------------------------------------------------------------------------
  // EN: Exercise EVERY public method of App on `a` (moved-from, impl_ == nullptr). Grouped in
  //     the SAME order app.hpp declares them, so a reviewer can diff this list against the
  //     header top-to-bottom.
  // PT: Exercita TODO método público de App em `a` (movido-de, impl_ == nullptr). Agrupado na
  //     MESMA ordem que app.hpp declara, para um reviewer diffar esta lista contra o header de
  //     cima a baixo.
  // ---------------------------------------------------------------------------------------

  expect("ok", a.ok() == false);

  expect("load", a.load("tests/min.rml") == false);

  a.set_dp_ratio(1.5f);
  expect("set_dp_ratio no-crash", true);

  a.set_asset_base_url("assets/");
  expect("set_asset_base_url no-crash", true);

  expect("set_window_mode", a.set_window_mode(WindowMode::Maximized) == false);
  expect("window_mode", a.window_mode() == WindowMode::Windowed);

  {
    int w = -1, h = -1;
    a.get_window_size(w, h);
    expect("get_window_size", w == 0 && h == 0);
  }

  {
    const unsigned char px[4] = {0, 0, 0, 255};
    expect("set_window_icon", a.set_window_icon(px, 1, 1) == false);
  }

  expect("running", a.running() == false);

  a.poll_events();
  expect("poll_events no-crash", true);

  a.update();
  expect("update no-crash", true);

  a.render();
  expect("render no-crash", true);

  // EN: run() loops `while (running())`; running() is false on a moved-from App (checked
  //     above), so this returns immediately without ever touching impl_ inside the loop body.
  // PT: run() laça `while (running())`; running() é false num App movido-de (checado acima),
  //     então isto retorna imediatamente sem nunca tocar impl_ dentro do corpo do laço.
  a.run();
  expect("run no-crash", true);

  {
    UiEvent ev{};
    ev.type = UiEvent::Type::MouseMove;
    a.process_event(ev);
    expect("process_event no-crash", true);
  }

  expect("is_key_down", a.is_key_down(Key::Up) == false);
  expect("is_mouse_button_down", a.is_mouse_button_down(0) == false);

  {
    float cx = -1.f, cy = -1.f;
    a.get_cursor_pos(cx, cy);
    expect("get_cursor_pos", cx == 0.f && cy == 0.f);
  }

  a.set_key_callback([](Key, KeyAction, int) {});
  expect("set_key_callback no-crash", true);

  a.request_close();
  expect("request_close no-crash", true);

  a.set_close_request_callback([]() { return true; });
  expect("set_close_request_callback no-crash", true);

  a.set_window_focus_callback([](bool) {});
  expect("set_window_focus_callback no-crash", true);

  a.set_window_iconify_callback([](bool) {});
  expect("set_window_iconify_callback no-crash", true);

  expect("is_window_focused", a.is_window_focused() == false);
  expect("is_window_iconified", a.is_window_iconified() == false);

  a.set_frame_callback([](float) {});
  expect("set_frame_callback no-crash", true);

  a.set_click_callback([](const char*) {});
  expect("set_click_callback no-crash", true);

  a.set_click_info_callback([](const ClickInfo&) {});
  expect("set_click_info_callback no-crash", true);

  a.set_scroll_callback([](const char*) {});
  expect("set_scroll_callback no-crash", true);

  a.set_change_callback([](const char*, const char*) {});
  expect("set_change_callback no-crash", true);

  a.set_submit_callback([](const char*) {});
  expect("set_submit_callback no-crash", true);

  a.set_focus_callback([](const char*) {});
  expect("set_focus_callback no-crash", true);

  a.set_blur_callback([](const char*) {});
  expect("set_blur_callback no-crash", true);

  a.set_hover_callback([](const char*, bool) {});
  expect("set_hover_callback no-crash", true);

  {
    ElementBox box = a.get_element_box("anything");
    expect("get_element_box", box.found == false && box.x == 0.f && box.y == 0.f &&
                                  box.w == 0.f && box.h == 0.f);
  }

  expect("scroll_element_into_view", a.scroll_element_into_view("anything") == false);

  {
    float out = -1.f;
    expect("get_element_scroll_top", a.get_element_scroll_top("anything", out) == false);
  }
  {
    float out = -1.f;
    expect("get_element_scroll_height", a.get_element_scroll_height("anything", out) == false);
  }
  {
    float out = -1.f;
    expect("get_element_client_height", a.get_element_client_height("anything", out) == false);
  }
  expect("set_element_scroll_top", a.set_element_scroll_top("anything", 10.f) == false);
  expect("set_focus", a.set_focus("anything") == false);
  expect("clear_focus", a.clear_focus() == false);

  expect("set_text", a.set_text("anything", "hello") == false);
  expect("add_class", a.add_class("anything", "cls") == false);
  expect("remove_class", a.remove_class("anything", "cls") == false);
  expect("set_property", a.set_property("anything", "color", "red") == false);

  {
    FontFaceDesc desc;
    desc.path = "assets/does-not-exist.ttf";
    expect("load_font_face", a.load_font_face(desc) == false);
  }

  expect("create_data_model", a.create_data_model("model") == false);
  expect("bind_number", a.bind_number("n", 1.0) == false);
  expect("bind_string", a.bind_string("s", "x") == false);
  expect("bind_bool", a.bind_bool("b", true) == false);
  expect("bind_list", a.bind_list("l") == false);

  a.set_number("n", 2.0);
  expect("set_number no-crash", true);

  a.set_string("s", "y");
  expect("set_string no-crash", true);

  a.set_bool("b", false);
  expect("set_bool no-crash", true);

  const char* items[1] = {"x"};
  a.set_list("l", items, 1);
  expect("set_list no-crash", true);

  {
    double dout = -1.0;
    expect("get_number", a.get_number("n", dout) == false);
  }
  {
    std::string sout = "unchanged";
    expect("get_string", a.get_string("s", sout) == false);
  }
  {
    bool bout = true;
    expect("get_bool", a.get_bool("b", bout) == false);
  }

  expect("snapshot", a.snapshot("/tmp/app_movedfrom_sanity_should_not_exist.ppm") == false);

  expect("set_swap_interval", a.set_swap_interval(1) == false);
  expect("set_vsync", a.set_vsync(true) == false);
  expect("get_monitor_refresh_hz", a.get_monitor_refresh_hz() == 0);

  {
    App::CapturedFrame frame = a.capture_frame();
    expect("capture_frame", frame.ok == false && frame.width == 0 && frame.height == 0 &&
                                frame.pixels == nullptr && frame.byte_count == 0);
  }

  // ---------------------------------------------------------------------------------------
  // EN: `b` (the move TARGET, still a fully-live App) must still work after `a`'s guard sites
  //     were all exercised -- a fix that broke `ready()`'s positive path (impl_ non-null, ok
  //     true) would pass every check above and still be wrong.
  // PT: `b` (o ALVO do move, ainda um App plenamente vivo) precisa continuar funcionando depois
  //     que todos os sítios de guarda de `a` foram exercitados -- um fix que quebrasse o
  //     caminho positivo de `ready()` (impl_ não-nulo, ok true) passaria em todo cheque acima e
  //     ainda assim estaria errado.
  // EN: NOT counted in g_exercised -- this checks `b`'s positive path, not `a`'s moved-from
  //     surface, so it must not skew the header-parity count below.
  // PT: NÃO contado em g_exercised -- isto checa o caminho positivo de `b`, não a superfície
  //     movida-de de `a`, então não pode enviesar a contagem de paridade com o header abaixo.
  if (!b.ok()) {
    std::fprintf(stderr, "app_movedfrom_sanity FAIL: post-move b.ok() should still be true\n");
    return 1;
  }
  b.poll_events();
  b.update();
  b.render();

  if (g_exercised != kExpectedPublicMethods) {
    std::fprintf(stderr,
                 "app_movedfrom_sanity FAIL: exercised %d methods, expected %d (App's public "
                 "surface in app.hpp changed -- update kExpectedPublicMethods AND the calls "
                 "above together)\n",
                 g_exercised, kExpectedPublicMethods);
    return 1;
  }

  if (g_failures != 0) {
    std::fprintf(stderr, "app_movedfrom_sanity FAIL: %d/%d checks failed\n", g_failures,
                 g_exercised);
    return 1;
  }

  std::printf(
      "app_movedfrom_sanity OK: %d public App methods verified fail-high on a "
      "moved-from App\n",
      g_exercised);
  return 0;
}
