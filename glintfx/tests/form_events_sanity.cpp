// SPDX-License-Identifier: MPL-2.0
// EN: form_events_sanity -- verifies the 5 L1.15-FORMEV callbacks (design doc
//     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Shape B + hover, per the
//     líder's decision): Engine::set_change_callback/set_submit_callback/set_focus_callback/
//     set_blur_callback/set_hover_callback, the exact code both glintfx::UiLayer and
//     glintfx::App delegate to with zero extra logic (straight passthrough, see
//     ui_layer.cpp/app.cpp) -- so this single test covers both public consumption paths, same
//     precedent as focus_sanity.cpp/domrw_sanity.cpp ("drive Engine directly for the internal
//     oracle").
//
//     Drives Engine (not UiLayer) directly and simulates input via Rml::Context::
//     ProcessMouseMove/ProcessMouseButtonDown/ProcessMouseButtonUp/ProcessMouseWheel directly
//     (Engine has no process_event() -- that translation layer is UiLayer-only), same
//     "RmlUi::Core oracle unreachable through UiLayer's type-free public header" reasoning as
//     focus_sanity.cpp/domrw_sanity.cpp.
//
//     EMPIRICAL FINDING (required by the task brief, confirmed here AND by reading
//     Context::OnFocusChange/SendEvents in the pinned RmlUi 6.3 source -- see
//     bootstrap.hpp's set_focus_callback doc-comment for the full writeup this test backs):
//       (1) Element::Focus() on an ALREADY-focused element is a TRUE NO-OP -- it dispatches
//           NEITHER Blur NOR Focus. RmlUi's own focus dedup is by ANCESTOR-CHAIN SET
//           DIFFERENCE (Context::OnFocusChange computes old_chain/new_chain BEFORE dispatch;
//           Context::SendEvents only dispatches to the set-difference); refocusing the exact
//           element that already holds Context::GetFocusElement() makes old_chain == new_chain,
//           so the difference is empty on both sides. Verified below (F3): a second
//           set_focus() call on the SAME already-focused id fires NEITHER focus_callback NOR
//           blur_callback (delta == 0 on both counters).
//       (2) A single set_focus() call transitioning from a FAR-AWAY old focus target (e.g. the
//           context's own root, or an unrelated sibling subtree) can fire focus_callback MORE
//           THAN ONCE -- Context::SendEvents dispatches ONE INDEPENDENT Focus event PER
//           ELEMENT newly entering the ancestor chain between the old and new focus target
//           (`std::set_difference` over the two chains, `element->DispatchEvent(...)` called
//           per element in the difference -- NOT a single bubbling event), each firing with
//           THAT element as target, each independently resolved by our own ancestor-walk. This
//           test's scene (#focus-outer wraps #focus-inner, both carry their own id) exercises
//           this directly: set_focus("focus-inner") fires focus_callback with id="focus-inner"
//           (asserted via CONTAINS, not an exact-count pin, since the exact fan-out size is an
//           RmlUi/std::set-ordering implementation detail, not the subject under test) alongside
//           possible extra firings for other newly-entered ancestors. clear_focus()
//           IMMEDIATELY AFTER a set_focus(), by contrast, is a CLEAN single dispatch (Blur only
//           re-parents focus UP to the immediate, already-in-chain parent -- see
//           Element::Blur() -- so the chain difference is exactly the one blurred leaf): F4
//           below asserts this exactly (delta == 1, id == "focus-inner").
//
// PT: form_events_sanity -- verifica os 5 callbacks do L1.15-FORMEV (doc de design
//     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Formato B + hover,
//     conforme decisão do líder): Engine::set_change_callback/set_submit_callback/
//     set_focus_callback/set_blur_callback/set_hover_callback, o código EXATO para o qual tanto
//     glintfx::UiLayer quanto glintfx::App encaminham sem lógica extra (repasse direto, ver
//     ui_layer.cpp/app.cpp) -- então este único teste cobre os dois caminhos públicos de
//     consumo, mesmo precedente de focus_sanity.cpp/domrw_sanity.cpp ("dirige o Engine
//     diretamente pelo oráculo interno").
//
//     Dirige o Engine (não o UiLayer) diretamente e simula input via Rml::Context::
//     ProcessMouseMove/ProcessMouseButtonDown/ProcessMouseButtonUp diretamente (o Engine não tem
//     process_event() -- essa camada de tradução é exclusiva do UiLayer), mesmo raciocínio de
//     "oráculo RmlUi::Core inalcançável pelo header público livre-de-tipos do UiLayer" de
//     focus_sanity.cpp/domrw_sanity.cpp.
//
//     ACHADO EMPÍRICO (exigido pelo brief da tarefa, confirmado aqui E lendo Context::
//     OnFocusChange/SendEvents no source pinado do RmlUi 6.3 -- ver o doc-comment de
//     set_focus_callback em bootstrap.hpp para o relato completo que este teste sustenta):
//       (1) Element::Focus() num elemento JÁ focado é um NO-OP VERDADEIRO -- não despacha NEM
//           Blur NEM Focus. O próprio dedup de foco do RmlUi é por DIFERENÇA DE CONJUNTO DA
//           CADEIA DE ANCESTRAIS (Context::OnFocusChange constrói old_chain/new_chain ANTES do
//           despacho; Context::SendEvents só despacha pra diferença de conjunto); refocar o
//           elemento exato que já detém Context::GetFocusElement() faz old_chain == new_chain,
//           então a diferença é vazia nos dois sentidos. Verificado abaixo (F3): uma segunda
//           chamada set_focus() no MESMO id já focado não dispara NEM focus_callback NEM
//           blur_callback (delta == 0 nos dois contadores).
//       (2) Uma única chamada set_focus() transicionando de um alvo de foco antigo DISTANTE
//           (ex.: a própria raiz do contexto, ou uma subárvore irmã não relacionada) PODE
//           disparar focus_callback MAIS DE UMA VEZ -- Context::SendEvents despacha UM evento
//           Focus INDEPENDENTE POR ELEMENTO que entra de novo na cadeia de ancestrais entre o
//           alvo de foco antigo e o novo (`std::set_difference` sobre as duas cadeias,
//           `element->DispatchEvent(...)` chamado por elemento na diferença -- NÃO um único
//           evento que borbulha), cada disparo com AQUELE elemento como alvo, cada um resolvido
//           independentemente pela nossa própria subida de ancestral. A cena deste teste
//           (#focus-outer envolve #focus-inner, ambos com id próprio) exercita isso diretamente:
//           set_focus("focus-inner") dispara focus_callback com id="focus-inner" (verificado via
//           CONTAINS, não um pino de contagem exata, já que o tamanho exato do fan-out é um
//           detalhe de implementação do RmlUi/ordenação de std::set, não o assunto sob teste)
//           junto de possíveis disparos extras para outros ancestrais recém-entrados.
//           clear_focus() IMEDIATAMENTE APÓS um set_focus(), em contraste, é um despacho único
//           LIMPO (Blur só reparenta o foco PRA CIMA pro pai imediato, já presente na cadeia --
//           ver Element::Blur() -- então a diferença de cadeia é exatamente a única folha
//           desfocada): F4 abaixo verifica isso exatamente (delta == 1, id == "focus-inner").
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <RmlUi/Core/Context.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

// EN: Simulates a left-click at (x, y): hover (MouseMove) + MouseButtonDown + MouseButtonUp on
//     the SAME point -- RmlUi's own click detection (down+up on the same element == Click),
//     same synthesis as click_callback_sanity.cpp's click_at() (there, via UiLayer::
//     process_event; here, directly against Rml::Context since Engine has no process_event()).
// PT: Simula um clique esquerdo em (x, y): hover (MouseMove) + MouseButtonDown + MouseButtonUp
//     no MESMO ponto -- a própria detecção de clique do RmlUi (down+up no mesmo elemento ==
//     Click), mesma síntese do click_at() de click_callback_sanity.cpp (lá, via UiLayer::
//     process_event; aqui, direto contra Rml::Context já que o Engine não tem process_event()).
static void click_at(Rml::Context* ctx, int x, int y) {
  ctx->ProcessMouseMove(x, y, 0);
  ctx->ProcessMouseButtonDown(0, 0);
  ctx->ProcessMouseButtonUp(0, 0);
}

template <typename T>
static bool contains(const std::vector<T>& v, const T& needle) {
  return std::find(v.begin(), v.end(), needle) != v.end();
}

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("form_events_host", 200, 220)) { std::puts("FAIL: host create"); return 1; }

  glintfx::SystemClock clock;
  glintfx::Engine engine;
  if (!engine.attach(&clock, 200, 220)) { std::puts("FAIL: engine attach"); return 2; }

  // ---------------------------------------------------------------------------
  // Recorders for all 5 channels -- registered BEFORE load(), same "no ordering constraint vs.
  // load()" contract as every callback in this family.
  // ---------------------------------------------------------------------------
  std::vector<std::pair<std::string, std::string>> change_hits;
  engine.set_change_callback([&](const char* id, const char* value) {
    change_hits.emplace_back(id ? id : "<null>", value ? value : "<null>");
  });

  std::vector<std::string> submit_hits;
  engine.set_submit_callback([&](const char* id) { submit_hits.emplace_back(id ? id : "<null>"); });

  std::vector<std::string> focus_hits;
  engine.set_focus_callback([&](const char* id) { focus_hits.emplace_back(id ? id : "<null>"); });

  std::vector<std::string> blur_hits;
  engine.set_blur_callback([&](const char* id) { blur_hits.emplace_back(id ? id : "<null>"); });

  std::vector<std::pair<std::string, bool>> hover_hits;
  engine.set_hover_callback([&](const char* id, bool entered) {
    hover_hits.emplace_back(id ? id : "<null>", entered);
  });

  if (!engine.load("form_events_scene.rml")) {
    std::puts("FAIL: load(form_events_scene.rml)"); return 3;
  }
  engine.update();

  Rml::Context* ctx = engine.context();
  if (!ctx) { std::puts("FAIL: null context after successful load"); return 4; }

  // ---------------------------------------------------------------------------
  // (F1) change -- clicking the checkbox (#chk, RCSS box (10,10)-(30,30), centre (20,20))
  //      toggles its "checked" attribute via InputTypeCheckbox::ProcessDefaultAction, which
  //      SYNCHRONOUSLY dispatches Change on the SAME element with the "value" parameter
  //      confirmed against the pinned RmlUi source (InputTypeCheckbox::OnAttributeChange):
  //      "yes" (the control's own value="yes" attribute) when checked, "" when unchecked.
  // ---------------------------------------------------------------------------
  click_at(ctx, 20, 20);
  if (change_hits.size() != 1) {
    std::fprintf(stderr, "FAIL(F1a): change_hits.size()=%zu expected 1 after checking\n",
                 change_hits.size());
    return 5;
  }
  if (change_hits[0].first != "chk" || change_hits[0].second != "yes") {
    std::fprintf(stderr, "FAIL(F1a): change_hits[0]=(\"%s\",\"%s\") expected (\"chk\",\"yes\")\n",
                 change_hits[0].first.c_str(), change_hits[0].second.c_str());
    return 6;
  }

  click_at(ctx, 20, 20); // uncheck
  if (change_hits.size() != 2) {
    std::fprintf(stderr, "FAIL(F1b): change_hits.size()=%zu expected 2 after unchecking\n",
                 change_hits.size());
    return 7;
  }
  if (change_hits[1].first != "chk" || change_hits[1].second != "") {
    std::fprintf(stderr, "FAIL(F1b): change_hits[1]=(\"%s\",\"%s\") expected (\"chk\",\"\")\n",
                 change_hits[1].first.c_str(), change_hits[1].second.c_str());
    return 8;
  }

  // ---------------------------------------------------------------------------
  // (F2) submit -- clicking the submit button (#submit-btn, RCSS box (10,40)-(70,60), centre
  //      (40,50)) triggers InputTypeSubmit::ProcessDefaultAction -> ElementForm::Submit(),
  //      which dispatches Submit ON THE FORM ELEMENT ITSELF (`this` inside Submit()) --
  //      confirmed against the pinned RmlUi source (ElementForm.cpp). The ancestor walk
  //      therefore resolves the form's OWN id, "myform", immediately (no walk needed).
  // ---------------------------------------------------------------------------
  click_at(ctx, 40, 50);
  if (submit_hits.size() != 1) {
    std::fprintf(stderr, "FAIL(F2): submit_hits.size()=%zu expected 1\n", submit_hits.size());
    return 9;
  }
  if (submit_hits[0] != "myform") {
    std::fprintf(stderr, "FAIL(F2): submit_hits[0]=\"%s\" expected \"myform\"\n",
                 submit_hits[0].c_str());
    return 10;
  }

  // ---------------------------------------------------------------------------
  // (F3) focus/blur -- Part 1: set_focus("focus-inner") reaches the NESTED target
  //      (#focus-outer wraps #focus-inner; the listener is attached at the DOCUMENT ROOT, in
  //      the CAPTURE phase -- proves capture-phase wiring closes the "Focus/Blur do not
  //      bubble" gap, see bootstrap.hpp's set_focus_callback doc-comment). CONTAINS, not an
  //      exact-count assertion -- see the file header EMPIRICAL FINDING (2) for why the exact
  //      fan-out size is not the subject under test here.
  // ---------------------------------------------------------------------------
  if (!engine.set_focus("focus-inner")) { std::puts("FAIL(F3a): set_focus(\"focus-inner\")"); return 11; }
  if (!contains(focus_hits, std::string("focus-inner"))) {
    std::puts("FAIL(F3a): focus_hits does not contain \"focus-inner\" -- capture-phase listener "
              "did not reach the nested target");
    return 12;
  }

  // ---------------------------------------------------------------------------
  // (F3b) EMPIRICAL FINDING (1) -- Element::Focus() on the ALREADY-focused element is a TRUE
  //       NO-OP: neither focus_callback nor blur_callback fires again. See the file header for
  //       the full Context::OnFocusChange/SendEvents ancestor-chain-set-difference reasoning.
  // ---------------------------------------------------------------------------
  {
    const size_t focus_before = focus_hits.size();
    const size_t blur_before = blur_hits.size();
    if (!engine.set_focus("focus-inner")) {
      std::puts("FAIL(F3b): set_focus(\"focus-inner\") on the already-focused element returned false");
      return 13;
    }
    if (focus_hits.size() != focus_before) {
      std::fprintf(stderr,
                   "FAIL(F3b): focus_hits grew (%zu -> %zu) on a same-element refocus -- expected "
                   "a TRUE no-op (empirical finding (1), see file header)\n",
                   focus_before, focus_hits.size());
      return 14;
    }
    if (blur_hits.size() != blur_before) {
      std::fprintf(stderr,
                   "FAIL(F3b): blur_hits grew (%zu -> %zu) on a same-element refocus -- expected "
                   "a TRUE no-op (empirical finding (1), see file header)\n",
                   blur_before, blur_hits.size());
      return 15;
    }
  }

  // ---------------------------------------------------------------------------
  // (F4) clear_focus() immediately after -- a CLEAN, single dispatch (see the file header
  //      EMPIRICAL FINDING (2) for why this case, unlike the initial set_focus(), is NOT
  //      fan-out-ambiguous): exactly ONE new blur_hits entry, id == "focus-inner", and NO new
  //      focus_hits entry (Blur only re-parents UP to the already-in-chain parent).
  // ---------------------------------------------------------------------------
  {
    const size_t focus_before = focus_hits.size();
    const size_t blur_before = blur_hits.size();
    if (!engine.clear_focus()) { std::puts("FAIL(F4): clear_focus() after set_focus"); return 16; }
    if (blur_hits.size() != blur_before + 1) {
      std::fprintf(stderr,
                   "FAIL(F4): blur_hits.size()=%zu expected exactly %zu (one clean dispatch)\n",
                   blur_hits.size(), blur_before + 1);
      return 17;
    }
    if (blur_hits.back() != "focus-inner") {
      std::fprintf(stderr, "FAIL(F4): blur_hits.back()=\"%s\" expected \"focus-inner\"\n",
                   blur_hits.back().c_str());
      return 18;
    }
    if (focus_hits.size() != focus_before) {
      std::fprintf(stderr,
                   "FAIL(F4): focus_hits grew (%zu -> %zu) from a clear_focus() call -- Blur "
                   "re-parenting to an already-in-chain parent must not fire a new Focus\n",
                   focus_before, focus_hits.size());
      return 19;
    }
  }

  // ---------------------------------------------------------------------------
  // (F5) hover -- dedup machine (Bootstrap::HoverEventListener, single Mouseover-only
  //      listener). #hover-a (10,80)-(110,120) contains an id-less <span> covering its LEFT
  //      half (10,80)-(60,120); the RIGHT half (60,80)-(110,120) is the plain #hover-a
  //      background. Moving between (30,90) [hits the span] and (80,90) [hits the div
  //      background directly] are TWO DIFFERENT Rml::Element targets that both resolve, via
  //      Context::GetHoverElement()'s ancestor walk, to the SAME id "hover-a" -- the dedup
  //      "dente": no re-fire while crossing that boundary. Moving on to #hover-b
  //      (10,130)-(110,160), point (50,145), is a genuinely different id: fires
  //      exit("hover-a", false) then enter("hover-b", true).
  //
  //      Baseline, not absolute indices/counts: F1/F2's click_at() calls above already moved
  //      the mouse (each click_at() hovers before pressing), so hover_hits already carries
  //      entries for "chk"/"submit-btn" by the time F5 starts, and the mouse's CURRENT hover
  //      target is "submit-btn" (F2's last click point), not "nothing" -- F5a's first move
  //      therefore fires an EXIT("submit-btn") + ENTER("hover-a") pair (2 new entries), not a
  //      bare enter. Capturing a size baseline right before F5 and asserting DELTAS/relative
  //      content keeps this test correct regardless of that incidental prior hover noise.
  // ---------------------------------------------------------------------------
  const size_t hover_before_f5 = hover_hits.size();

  ctx->ProcessMouseMove(30, 90, 0); // enter #hover-a via the span child (from #submit-btn)
  if (hover_hits.size() != hover_before_f5 + 2) {
    std::fprintf(stderr,
                 "FAIL(F5a): hover_hits.size()=%zu expected %zu (exit submit-btn + enter hover-a)\n",
                 hover_hits.size(), hover_before_f5 + 2);
    return 20;
  }
  if (hover_hits[hover_before_f5].second != false) {
    std::fprintf(stderr, "FAIL(F5a): exit entry second=%d expected false\n",
                 (int)hover_hits[hover_before_f5].second);
    return 21;
  }
  if (hover_hits.back().first != "hover-a" || hover_hits.back().second != true) {
    std::fprintf(stderr, "FAIL(F5a): hover_hits.back()=(\"%s\",%d) expected (\"hover-a\",1)\n",
                 hover_hits.back().first.c_str(), (int)hover_hits.back().second);
    return 22;
  }

  ctx->ProcessMouseMove(80, 90, 0); // still inside #hover-a, but over its OWN background now
  if (hover_hits.size() != hover_before_f5 + 2) {
    std::fprintf(stderr,
                 "FAIL(F5b, DEDUP): hover_hits.size()=%zu expected still %zu -- crossing an "
                 "unlabeled child's boundary inside the SAME labeled container must not re-fire\n",
                 hover_hits.size(), hover_before_f5 + 2);
    return 23;
  }

  ctx->ProcessMouseMove(50, 145, 0); // #hover-b -- genuinely different id
  if (hover_hits.size() != hover_before_f5 + 4) {
    std::fprintf(stderr,
                 "FAIL(F5c): hover_hits.size()=%zu expected %zu (exit hover-a + enter hover-b)\n",
                 hover_hits.size(), hover_before_f5 + 4);
    return 24;
  }
  if (hover_hits[hover_before_f5 + 2].first != "hover-a" ||
      hover_hits[hover_before_f5 + 2].second != false) {
    std::fprintf(stderr, "FAIL(F5c): exit entry=(\"%s\",%d) expected (\"hover-a\",0)\n",
                 hover_hits[hover_before_f5 + 2].first.c_str(),
                 (int)hover_hits[hover_before_f5 + 2].second);
    return 25;
  }
  if (hover_hits.back().first != "hover-b" || hover_hits.back().second != true) {
    std::fprintf(stderr, "FAIL(F5c): hover_hits.back()=(\"%s\",%d) expected (\"hover-b\",1)\n",
                 hover_hits.back().first.c_str(), (int)hover_hits.back().second);
    return 26;
  }

  // ---------------------------------------------------------------------------
  // (F6) Reentrancy (AUD-TEC-3) -- submit_callback re-registers itself from inside its own
  //      invocation (same "swap the handler mid-dispatch" pattern proven for
  //      set_click_callback in click_callback_sanity.cpp): without SubmitEventListener::
  //      ProcessEvent's local-copy-before-invoke guard, the std::move-assignment inside the
  //      nested set_submit_callback() call would destroy the std::function target (this very
  //      closure) while ProcessEvent is still invoking it through the cb_ pointer -- run under
  //      ASan to catch a regression.
  // ---------------------------------------------------------------------------
  std::vector<std::string> reentrant_submit_hits;
  bool submit_swapped = false;
  engine.set_submit_callback([&](const char* id) {
    reentrant_submit_hits.emplace_back(id ? id : "<null>");
    if (!submit_swapped) {
      submit_swapped = true;
      engine.set_submit_callback([&reentrant_submit_hits](const char* id2) {
        reentrant_submit_hits.emplace_back(std::string("second:") + (id2 ? id2 : "<null>"));
      });
      submit_swapped = true; // re-derefs `submit_swapped` through this closure's own storage.
    }
  });
  click_at(ctx, 40, 50); // 1st handler fires, swaps itself out mid-invocation
  click_at(ctx, 40, 50); // 2nd handler (installed above) now fires
  if (reentrant_submit_hits.size() != 2) {
    std::fprintf(stderr, "FAIL(F6a): reentrant_submit_hits.size()=%zu expected 2\n",
                 reentrant_submit_hits.size());
    return 27;
  }
  if (reentrant_submit_hits[0] != "myform" || reentrant_submit_hits[1] != "second:myform") {
    std::fprintf(stderr, "FAIL(F6a): reentrant_submit_hits=[\"%s\",\"%s\"] expected "
                 "[\"myform\",\"second:myform\"]\n",
                 reentrant_submit_hits[0].c_str(), reentrant_submit_hits[1].c_str());
    return 28;
  }

  // EN: (F6b) Same reentrancy proof for hover -- HoverEventListener carries EXTRA state
  //     (current_hover_id_) beyond a bare cb_ pointer, so this exercises a distinct code path
  //     from F6a above: a handler that re-registers set_hover_callback from inside its own
  //     invocation must not corrupt/UAF the local `cb` copy OR the dedup state.
  // PT: (F6b) Mesma prova de reentrância pro hover -- HoverEventListener carrega estado EXTRA
  //     (current_hover_id_) além de um simples ponteiro cb_, então isto exercita um caminho de
  //     código distinto do F6a acima: um handler que re-registra set_hover_callback de dentro
  //     da própria invocação não pode corromper/UAF a cópia local `cb` NEM o estado de dedup.
  std::vector<std::string> reentrant_hover_hits;
  bool hover_swapped = false;
  engine.set_hover_callback([&](const char* id, bool entered) {
    reentrant_hover_hits.emplace_back(
        (entered ? std::string("+") : std::string("-")) + (id ? id : "<null>"));
    if (!hover_swapped) {
      hover_swapped = true;
      engine.set_hover_callback([&reentrant_hover_hits](const char* id2, bool entered2) {
        reentrant_hover_hits.emplace_back(
            std::string("second:") + (entered2 ? "+" : "-") + (id2 ? id2 : "<null>"));
      });
      hover_swapped = true;
    }
  });
  ctx->ProcessMouseMove(20, 20, 0);  // to #chk (id "chk") -- different from hover-b, 1st handler
                                     // fires (exit hover-b, enter chk), swaps mid-invocation
  ctx->ProcessMouseMove(50, 145, 0); // back to #hover-b -- 2nd handler (installed above) now fires
  ctx->ProcessMouseMove(30, 90, 0);  // #hover-a -- 2nd handler fires again
  if (reentrant_hover_hits.empty()) {
    std::puts("FAIL(F6b): reentrant_hover_hits is empty"); return 29;
  }
  if (reentrant_hover_hits.back().rfind("second:", 0) != 0) {
    std::fprintf(stderr,
                 "FAIL(F6b): reentrant_hover_hits.back()=\"%s\" expected a \"second:\"-prefixed "
                 "entry (2nd handler installed by the 1st did not survive to fire)\n",
                 reentrant_hover_hits.back().c_str());
    return 30;
  }

  if (!engine.ok()) { std::puts("FAIL: ok() false at end of test"); return 31; }

  std::puts("form_events_sanity: PASS");
  return 0;
}
