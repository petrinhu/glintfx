// SPDX-License-Identifier: Apache-2.0
// EN: AUD-UILAYER-MOVEDFROM -- the UiLayer twin of app_movedfrom_sanity.cpp (AUD-APP-MOVEDFROM,
//     W25). ENUMERATES every public UiLayer method (not a sample) and calls each one on a
//     MOVED-FROM UiLayer (`impl_ == nullptr`), asserting fail-high (the exact value each method
//     already documents/returns for `!ok()`), never a crash. Before the fix this file exists to
//     drive, the guard pattern throughout ui_layer.cpp is `if (!impl_->ok) ...`, which
//     dereferences `impl_` BEFORE checking whether it is null -- the RED this test produces
//     pre-fix is exactly that segfault: ctest reports a crashed process, not a clean assertion
//     failure -- expected, and the proof the guard bites (house rule 2026-07-27: "enumeration
//     finds what you didn't know to suspect" -- a sampled subset of methods could pass while
//     the un-sampled ones still segfault).
//
//     UiLayer-SPECIFIC EXTRA: render()/update() on a moved-from UiLayer are NOT just required
//     to not crash -- this class is the EMBED facade, and the GL context render()/update()
//     would otherwise touch belongs to the HOST, not to glintfx. Part G below sets a
//     non-default, easy-to-notice GL state (viewport, blend equation/func, scissor, program,
//     VAO, draw-FBO binding, colour mask, cull/stencil enables, read buffer -- the SAME
//     checklist gl_state_guard.cpp already uses to prove render()'s own GlStateGuard restores
//     everything it touches), snapshots it, calls update() then render() on the MOVED-FROM
//     object, re-snapshots, and asserts byte-identical: a moved-from no-op that still issued
//     ANY GL call (even a harmless one) would be glintfx corrupting a host's graphics state
//     behind its back, worse than a crash because it fails silently. This is the empirical
//     proof the ready() guard short-circuits BEFORE Engine::update()/Engine::render_compose()
//     (and the GlStateGuard construction inside the latter) are ever reached -- not just an
//     inference from reading the source.
//
//     METHOD COUNT (paridade com o header): kExpectedPublicMethods below is the hand-verified
//     count of glintfx::UiLayer's public member functions in glintfx/include/glintfx/
//     ui_layer.hpp, EXCLUDING the four special members (constructor, destructor, move
//     constructor, move assignment -- inherently safe on a moved-from object already: the dtor
//     is a no-op on a null impl_, and the move ops accept a moved-from source by construction).
//     ok() IS included (it is a public method, and it is the one site the class already made
//     null-safe -- exercising it here proves the enumeration is complete, not merely "every
//     method except the one that already worked"). Counted 2026-08-04 against ui_layer.hpp: 43.
//     If a future UiLayer method is added/removed, this constant AND the calls below must be
//     updated together -- a mismatch fails loudly (see main()'s final check) instead of
//     silently under-covering the surface.
// PT: AUD-UILAYER-MOVEDFROM -- o gêmeo do app_movedfrom_sanity.cpp (AUD-APP-MOVEDFROM, W25)
//     para o UiLayer. ENUMERA todo método público de UiLayer (não uma amostra) e chama cada um
//     num UiLayer MOVIDO-DE (`impl_ == nullptr`), verificando fail-high (o valor exato que cada
//     método já documenta/devolve para `!ok()`), nunca um crash. Antes do fix que este arquivo
//     existe para dirigir, o padrão de guarda por todo ui_layer.cpp é `if (!impl_->ok) ...`, que
//     derreferencia `impl_` ANTES de checar se é nulo -- o VERMELHO que este teste produz
//     pré-fix é exatamente esse segfault: o ctest reporta um processo que crashou, não uma
//     falha de assert limpa -- esperado, e a prova de que a guarda morde (regra da casa
//     2026-07-27: "enumeração encontra o que você não sabia que devia suspeitar" -- uma amostra
//     de métodos podia passar enquanto os não-amostrados ainda segfaltavam).
//
//     EXTRA ESPECÍFICO DO UiLayer: render()/update() num UiLayer movido-de NÃO precisam só de
//     não crashar -- esta classe é a fachada EMBED, e o contexto GL que render()/update()
//     tocariam pertence ao HOST, não à glintfx. A Parte G abaixo define um estado GL
//     não-default, fácil de notar (viewport, equação/func de blend, scissor, programa, VAO,
//     binding de draw-FBO, color mask, enables cull/stencil, read buffer -- a MESMA checklist
//     que gl_state_guard.cpp já usa para provar que o próprio GlStateGuard do render() restaura
//     tudo que toca), tira um snapshot, chama update() e depois render() no objeto MOVIDO-DE,
//     tira outro snapshot, e verifica byte-idêntico: um no-op movido-de que ainda emitisse
//     QUALQUER chamada GL (mesmo inofensiva) seria a glintfx corrompendo o estado gráfico de um
//     host por trás dele, pior que um crash porque falha em silêncio. Esta é a prova empírica
//     de que a guarda ready() intercepta ANTES de Engine::update()/Engine::render_compose() (e
//     a construção do GlStateGuard dentro deste último) serem sequer alcançados -- não só uma
//     inferência lendo o source.
//
//     CONTAGEM DE MÉTODOS (paridade com o header): kExpectedPublicMethods abaixo é a contagem
//     verificada à mão dos métodos-membro públicos de glintfx::UiLayer em
//     glintfx/include/glintfx/ui_layer.hpp, EXCLUINDO os quatro membros especiais (construtor,
//     destrutor, construtor de move, atribuição de move -- já inerentemente seguros num objeto
//     movido-de: o destrutor é no-op sobre um impl_ nulo, e as operações de move aceitam uma
//     origem movida-de por construção). ok() ESTÁ incluído (é um método público, e é o único
//     sítio que a classe já tornava null-safe -- exercitá-lo aqui prova que a enumeração está
//     completa, não apenas "todo método exceto o que já funcionava"). Contado em 2026-08-04
//     contra ui_layer.hpp: 43. Se um método futuro de UiLayer for adicionado/removido, esta
//     constante E as chamadas abaixo precisam ser atualizadas juntas -- uma divergência falha
//     ruidosamente (ver o cheque final do main()) em vez de sub-cobrir a superfície
//     silenciosamente.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h. PT: inclui gl_loader.h.
#include <cstdio>
#include <cstring>
#include <utility>

using namespace glintfx;

namespace {

constexpr int kExpectedPublicMethods = 43;

int g_exercised = 0;
int g_failures = 0;

void expect(const char* method, bool condition) {
  ++g_exercised;
  if (!condition) {
    ++g_failures;
    std::fprintf(stderr,
                 "ui_layer_movedfrom_sanity FAIL: %s did not fail-high on a moved-from UiLayer\n",
                 method);
  }
}

// EN: Part G helper -- the SAME GL-state checklist gl_state_guard.cpp uses to prove render()'s
//     own GlStateGuard restores everything it touches, captured here to PROVE THE OPPOSITE
//     claim: on a moved-from UiLayer, update()/render() touch NONE of it (the guard short-
//     circuits before Engine::update()/Engine::render_compose() -- and the GlStateGuard
//     construction inside the latter -- are ever reached). One capture() covers the full
//     checklist so Part G below can snapshot THREE times (pre-update, post-update/pre-render,
//     post-render) and attribute a divergence to the ONE call that produced it, without
//     duplicating ~25 glGet* calls by hand three times.
// PT: Helper da Parte G -- a MESMA checklist de estado GL que gl_state_guard.cpp usa para
//     provar que o próprio GlStateGuard do render() restaura tudo que toca, capturada aqui para
//     PROVAR A AFIRMAÇÃO OPOSTA: num UiLayer movido-de, update()/render() não tocam NADA dela
//     (a guarda intercepta antes de Engine::update()/Engine::render_compose() -- e a construção
//     do GlStateGuard dentro deste último -- serem sequer alcançados). Um único capture() cobre
//     a checklist inteira para que a Parte G abaixo tire TRÊS snapshots (pré-update,
//     pós-update/pré-render, pós-render) e atribua uma divergência à ÚNICA chamada que a
//     produziu, sem duplicar ~25 chamadas glGet* à mão três vezes.
struct GlSnapshot {
  GLint viewport[4] = {};
  GLint scissor_box[4] = {};
  GLint blend = 0, scissor_test = 0, depth_test = 0;
  GLint program = 0, vao = 0, draw_fbo = 0;
  GLint blend_eq_rgb = 0, blend_eq_a = 0;
  GLboolean color_mask[4] = {};
  GLint blend_src_rgb = 0, blend_dst_rgb = 0, blend_src_a = 0, blend_dst_a = 0;
  GLint cull_face = 0, stencil_test = 0, read_buffer = 0;
  GLenum gl_error = GL_NO_ERROR;
};

GlSnapshot capture_gl_state() {
  GlSnapshot s;
  glGetIntegerv(GL_VIEWPORT, s.viewport);
  glGetIntegerv(GL_SCISSOR_BOX, s.scissor_box);
  s.blend = (GLint)glIsEnabled(GL_BLEND);
  s.scissor_test = (GLint)glIsEnabled(GL_SCISSOR_TEST);
  s.depth_test = (GLint)glIsEnabled(GL_DEPTH_TEST);
  glGetIntegerv(GL_CURRENT_PROGRAM, &s.program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &s.vao);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &s.draw_fbo);
  glGetIntegerv(GL_BLEND_EQUATION_RGB, &s.blend_eq_rgb);
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &s.blend_eq_a);
  glGetBooleanv(GL_COLOR_WRITEMASK, s.color_mask);
  glGetIntegerv(GL_BLEND_SRC_RGB, &s.blend_src_rgb);
  glGetIntegerv(GL_BLEND_DST_RGB, &s.blend_dst_rgb);
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &s.blend_src_a);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &s.blend_dst_a);
  s.cull_face = (GLint)glIsEnabled(GL_CULL_FACE);
  s.stencil_test = (GLint)glIsEnabled(GL_STENCIL_TEST);
  glGetIntegerv(GL_READ_BUFFER, &s.read_buffer);
  s.gl_error = glGetError();
  return s;
}

// EN: Compares two snapshots, printing every diverging field (not just the first) tagged with
//     `call_name` so a failure log pinpoints exactly which of update()/render() touched GL.
//     Returns true when byte-identical.
// PT: Compara dois snapshots, imprimindo todo campo divergente (não só o primeiro) marcado com
//     `call_name` para que um log de falha aponte exatamente qual de update()/render() tocou
//     GL. Retorna true quando byte-idêntico.
bool gl_state_unchanged(const char* call_name, const GlSnapshot& pre, const GlSnapshot& post) {
  bool ok = true;
#define CHK(field_name, pre_val, post_val)                                           \
  if ((pre_val) != (post_val)) {                                                     \
    std::fprintf(stderr,                                                             \
                 "ui_layer_movedfrom_sanity FAIL: GL %s changed by a moved-from %s " \
                 "(before=%d after=%d)\n",                                           \
                 (field_name), call_name, (int)(pre_val), (int)(post_val));          \
    ok = false;                                                                      \
  }
  CHK("GL_VIEWPORT[0]", pre.viewport[0], post.viewport[0]);
  CHK("GL_VIEWPORT[1]", pre.viewport[1], post.viewport[1]);
  CHK("GL_VIEWPORT[2]", pre.viewport[2], post.viewport[2]);
  CHK("GL_VIEWPORT[3]", pre.viewport[3], post.viewport[3]);
  CHK("GL_SCISSOR_BOX[0]", pre.scissor_box[0], post.scissor_box[0]);
  CHK("GL_SCISSOR_BOX[1]", pre.scissor_box[1], post.scissor_box[1]);
  CHK("GL_SCISSOR_BOX[2]", pre.scissor_box[2], post.scissor_box[2]);
  CHK("GL_SCISSOR_BOX[3]", pre.scissor_box[3], post.scissor_box[3]);
  CHK("GL_BLEND", pre.blend, post.blend);
  CHK("GL_SCISSOR_TEST", pre.scissor_test, post.scissor_test);
  CHK("GL_DEPTH_TEST", pre.depth_test, post.depth_test);
  CHK("GL_CURRENT_PROGRAM", pre.program, post.program);
  CHK("GL_VERTEX_ARRAY_BINDING", pre.vao, post.vao);
  CHK("GL_DRAW_FRAMEBUFFER_BINDING", pre.draw_fbo, post.draw_fbo);
  CHK("GL_BLEND_EQUATION_RGB", pre.blend_eq_rgb, post.blend_eq_rgb);
  CHK("GL_BLEND_EQUATION_ALPHA", pre.blend_eq_a, post.blend_eq_a);
  CHK("GL_COLOR_WRITEMASK[0]", pre.color_mask[0], post.color_mask[0]);
  CHK("GL_COLOR_WRITEMASK[1]", pre.color_mask[1], post.color_mask[1]);
  CHK("GL_COLOR_WRITEMASK[2]", pre.color_mask[2], post.color_mask[2]);
  CHK("GL_COLOR_WRITEMASK[3]", pre.color_mask[3], post.color_mask[3]);
  CHK("GL_BLEND_SRC_RGB", pre.blend_src_rgb, post.blend_src_rgb);
  CHK("GL_BLEND_DST_RGB", pre.blend_dst_rgb, post.blend_dst_rgb);
  CHK("GL_BLEND_SRC_ALPHA", pre.blend_src_a, post.blend_src_a);
  CHK("GL_BLEND_DST_ALPHA", pre.blend_dst_a, post.blend_dst_a);
  CHK("GL_CULL_FACE", pre.cull_face, post.cull_face);
  CHK("GL_STENCIL_TEST", pre.stencil_test, post.stencil_test);
  CHK("GL_READ_BUFFER", pre.read_buffer, post.read_buffer);
  CHK("glGetError()", pre.gl_error, post.gl_error);
#undef CHK
  return ok;
}

} // namespace

int main() {
  // EN: Host creates a hidden window + current GL context -- the ONE live GL context this
  //     whole process ever has (UILAYER-SINGLETON, ui_layer.hpp: two UiLayer instances alive
  //     at once corrupt global RmlUi state). Only `live` below ever holds a constructed Impl at
  //     any point in time: `a` is move-CONSTRUCTED from `live` further down, which leaves
  //     `live` moved-from immediately -- exactly the same "one Impl, one owner, transferred not
  //     duplicated" shape app_movedfrom_sanity.cpp already established for App.
  // PT: Host cria janela oculta + contexto GL corrente -- o ÚNICO contexto GL vivo que este
  //     processo inteiro tem (UILAYER-SINGLETON, ui_layer.hpp: duas instâncias de UiLayer vivas
  //     ao mesmo tempo corrompem estado global do RmlUi). Só `live` abaixo chega a deter um
  //     Impl construído em algum momento: `a` é move-CONSTRUÍDA de `live` mais adiante, o que
  //     deixa `live` movida-de imediatamente -- exatamente a mesma forma "um Impl, um dono,
  //     transferido não duplicado" que app_movedfrom_sanity.cpp já estabeleceu para App.
  glintfx::WindowGlfw host;
  if (!host.create("ui_layer_movedfrom_sanity", 320, 240)) {
    std::fprintf(stderr, "ui_layer_movedfrom_sanity SETUP FAIL: host window create failed\n");
    return 1;
  }

  UiLayer live({.logical_width = 320, .logical_height = 240});
  if (!live.ok()) {
    std::fprintf(stderr,
                 "ui_layer_movedfrom_sanity SETUP FAIL: UiLayer construction failed "
                 "(ok()==false before any move)\n");
    return 1;
  }

  // EN: The move that puts `a` in the moved-from state under test. `b` stays alive (and owns
  //     the real Engine/RmlUi Context attached to the host's GL context) for the whole test so
  //     the process has a valid UiLayer to destruct cleanly at scope exit, same as every other
  //     UiLayer test in this suite.
  // PT: O move que coloca `a` no estado movido-de sob teste. `b` fica viva (e é dona do
  //     Engine/Context RmlUi reais anexados ao contexto GL do host) pelo teste inteiro, para o
  //     processo ter um UiLayer válido para destruir de forma limpa na saída de escopo, igual a
  //     todo outro teste de UiLayer desta suíte.
  UiLayer a(std::move(live));
  if (!a.ok()) {
    std::fprintf(stderr, "ui_layer_movedfrom_sanity SETUP FAIL: move target a.ok()==false\n");
    return 1;
  }
  UiLayer b(std::move(a));
  // EN: `a` is now moved-from (impl_ == nullptr); `b` owns the live Engine.
  // PT: `a` agora está movida-de (impl_ == nullptr); `b` é dona do Engine vivo.

  // ---------------------------------------------------------------------------------------
  // EN: Part G -- snapshot GL state, exercise render()/update() on the MOVED-FROM `a`, then
  //     re-snapshot and assert byte-identical. Runs BEFORE the rest of the enumeration so a
  //     divergence is attributed unambiguously to render()/update() alone, not to some other
  //     method's side effect. Non-default sentinels chosen the same way gl_state_guard.cpp
  //     chose them: distinguishable from BOTH the GL default AND from whatever RmlUi's own
  //     BeginFrame()/EndFrame() would set if (bug still present) it actually ran.
  // PT: Parte G -- snapshot de estado GL, exercita render()/update() no `a` MOVIDO-DE, então
  //     re-tira o snapshot e verifica byte-idêntico. Roda ANTES do resto da enumeração para que
  //     uma divergência seja atribuída sem ambiguidade só a render()/update(), não a efeito
  //     colateral de outro método. Sentinelas não-default escolhidas do mesmo jeito que
  //     gl_state_guard.cpp escolheu: distinguíveis tanto do default do GL quanto do que o
  //     próprio BeginFrame()/EndFrame() do RmlUi definiria se (bug ainda presente) ele de fato
  //     rodasse.
  // ---------------------------------------------------------------------------------------
  {
    gtest_off::Fbo fbo = gtest_off::make_fbo(320, 240);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // EN: Non-default sentinel state -- distinguishable from BOTH the GL default AND from
    //     whatever RmlUi's own BeginFrame()/EndFrame() would set if (bug still present) it
    //     actually ran (same choices gl_state_guard.cpp already made for the SAME reason).
    // PT: Estado sentinela não-default -- distinguível tanto do default do GL quanto do que o
    //     próprio BeginFrame()/EndFrame() do RmlUi definiria se (bug ainda presente) ele de
    //     fato rodasse (mesmas escolhas que gl_state_guard.cpp já fez pelo MESMO motivo).
    glViewport(3, 5, 111, 77);
    glEnable(GL_SCISSOR_TEST);
    glScissor(1, 2, 3, 4);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glBlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_STENCIL_TEST);
    glReadBuffer(GL_NONE);

    // EN: THREE snapshots (not two): pre-update, mid (post-update/pre-render), post-render.
    //     This attributes a divergence to the ONE call that produced it -- update() and
    //     render() are two separate public methods in the header's own enumeration order, and
    //     each needs its OWN expect() below to keep g_exercised == kExpectedPublicMethods (a
    //     single combined "update+render" check would double up on nothing, but calling
    //     expect() once for a pair of methods under-counts by one against the header's own
    //     method count).
    // PT: TRÊS snapshots (não dois): pré-update, meio (pós-update/pré-render), pós-render.
    //     Isto atribui uma divergência à ÚNICA chamada que a produziu -- update() e render()
    //     são dois métodos públicos separados na própria ordem de enumeração do header, e cada
    //     um precisa do PRÓPRIO expect() abaixo para manter g_exercised ==
    //     kExpectedPublicMethods (um cheque combinado único "update+render" não duplicaria
    //     nada, mas chamar expect() uma vez para um par de métodos sub-contaria em um contra a
    //     própria contagem de métodos do header).
    GlSnapshot pre = capture_gl_state();
    a.update();
    GlSnapshot mid = capture_gl_state();
    a.render();
    GlSnapshot post = capture_gl_state();

    expect("update no GL touch + no-crash", gl_state_unchanged("update()", pre, mid));
    expect("render no GL touch + no-crash", gl_state_unchanged("render()", mid, post));

    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo.fbo);
    glDeleteTextures(1, &fbo.tex);
  }

  // ---------------------------------------------------------------------------------------
  // EN: Exercise EVERY remaining public method of UiLayer on `a` (moved-from, impl_ ==
  //     nullptr). Grouped in the SAME order ui_layer.hpp declares them, so a reviewer can diff
  //     this list against the header top-to-bottom. ok()/update()/render() come first to match
  //     the header, but update()/render() were already exercised (with GL-state proof) in
  //     Part G above -- calling them again here would double-count g_exercised, so they are
  //     represented ONLY by the two "no-crash" expect() calls already recorded in Part G.
  // PT: Exercita TODO método público restante de UiLayer em `a` (movido-de, impl_ == nullptr).
  //     Agrupado na MESMA ordem que ui_layer.hpp declara, para um reviewer diffar esta lista
  //     contra o header de cima a baixo. ok()/update()/render() vêm primeiro para casar com o
  //     header, mas update()/render() já foram exercitados (com prova de estado GL) na Parte G
  //     acima -- chamá-los de novo aqui contaria g_exercised em dobro, então são representados
  //     SÓ pelas duas chamadas expect() "no-crash" já registradas na Parte G.
  // ---------------------------------------------------------------------------------------

  expect("ok", a.ok() == false);

  expect("load", a.load("tests/min.rml") == false);

  a.set_viewport(800, 600);
  expect("set_viewport(w,h) no-crash", true);

  a.set_viewport(1, 2, 800, 600, 600);
  expect("set_viewport(x,y,w,h,target_h) no-crash", true);

  a.set_dp_ratio(2.0f);
  expect("set_dp_ratio no-crash", true);

  a.set_asset_base_url("assets/");
  expect("set_asset_base_url no-crash", true);

  // EN: update()/render() -- accounted for in Part G above (2 expect() calls already made).
  // PT: update()/render() -- contabilizados na Parte G acima (2 chamadas expect() já feitas).

  {
    UiEvent ev{};
    ev.type = UiEvent::Type::MouseMove;
    a.process_event(ev);
    expect("process_event no-crash", true);
  }

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

  {
    UiLayer::CapturedFrame frame = a.capture_frame();
    expect("capture_frame", frame.ok == false && frame.width == 0 && frame.height == 0 &&
                                frame.pixels == nullptr && frame.byte_count == 0);
  }

  // ---------------------------------------------------------------------------------------
  // EN: `b` (the move TARGET, still a fully-live UiLayer) must still work after `a`'s guard
  //     sites were all exercised -- a fix that broke ready()'s positive path (impl_ non-null,
  //     ok true) would pass every check above and still be wrong.
  // PT: `b` (o ALVO do move, ainda um UiLayer plenamente vivo) precisa continuar funcionando
  //     depois que todos os sítios de guarda de `a` foram exercitados -- um fix que quebrasse o
  //     caminho positivo de ready() (impl_ não-nulo, ok true) passaria em todo cheque acima e
  //     ainda assim estaria errado.
  // EN: NOT counted in g_exercised -- this checks `b`'s positive path, not `a`'s moved-from
  //     surface, so it must not skew the header-parity count below.
  // PT: NÃO contado em g_exercised -- isto checa o caminho positivo de `b`, não a superfície
  //     movida-de de `a`, então não pode enviesar a contagem de paridade com o header abaixo.
  // ---------------------------------------------------------------------------------------
  if (!b.ok()) {
    std::fprintf(stderr, "ui_layer_movedfrom_sanity FAIL: post-move b.ok() should still be true\n");
    return 1;
  }
  b.load("tests/min.rml");
  b.update();
  b.render();

  if (g_exercised != kExpectedPublicMethods) {
    std::fprintf(stderr,
                 "ui_layer_movedfrom_sanity FAIL: exercised %d methods, expected %d (UiLayer's "
                 "public surface in ui_layer.hpp changed -- update kExpectedPublicMethods AND "
                 "the calls above together)\n",
                 g_exercised, kExpectedPublicMethods);
    return 1;
  }

  if (g_failures != 0) {
    std::fprintf(stderr, "ui_layer_movedfrom_sanity FAIL: %d/%d checks failed\n", g_failures,
                 g_exercised);
    return 1;
  }

  std::printf(
      "ui_layer_movedfrom_sanity OK: %d public UiLayer methods verified fail-high on a "
      "moved-from UiLayer\n",
      g_exercised);
  return 0;
}
