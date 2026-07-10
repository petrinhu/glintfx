// SPDX-License-Identifier: MPL-2.0
// EN: focus_sanity -- verifies Engine::set_focus()/clear_focus() (L1.17-FOCUS, consumes the
//     GAP-4 seed from docs/embed-integration.md section 5), the exact code
//     glintfx::UiLayer::set_focus/clear_focus and glintfx::App::set_focus/clear_focus both
//     delegate to with zero extra logic (straight passthrough, see ui_layer.cpp/app.cpp) -- so
//     this single test covers both public consumption paths, same reasoning/precedent as
//     document_reload_leak.cpp's "drive Engine directly for the internal oracle" pattern.
//
//     Drives Engine (not UiLayer) directly because the ORACLE needed here --
//     Rml::Context::GetFocusElement() -- is only reachable from a test that includes
//     RmlUi/Core/Context.h + RmlUi/Core/Element.h directly; UiLayer's public header
//     deliberately exposes no Rml:: types (encapsulation gate) and has no context() accessor
//     for tests to hook into.
//
//     IMPORTANT CORRECTION vs. naive HTML-tabindex assumptions (confirmed by reading the pinned
//     RmlUi 6.3 source, NOT assumed): Rml::Element::Focus() gates ONLY on the element's RCSS
//     `focus` property (Style::Focus, keyword `auto`|`none`), whose RmlUi-REGISTERED DEFAULT IS
//     `auto` (StyleSheetSpecification.cpp) -- i.e. by default EVERY element (including a plain
//     unstyled <div>) accepts Focus(), UNLESS explicitly authored `focus: none;`. The separate
//     `tab-index` RCSS property (default `none`) controls ONLY Tab-key TRAVERSAL ORDER and has
//     ZERO effect on Focus()'s own success/failure -- there is no RmlUi/glintfx equivalent of
//     HTML's implicit "only tabindex-bearing elements are focusable" rule. focus_scene.rcss
//     therefore authors the REAL gate directly: #focusable stays at the registered default
//     (focus: auto, plus tab-index: auto to mirror how a real host would mark up a genuinely
//     Tab-reachable item), #unfocusable explicitly overrides `focus: none;`.
//
//     Also confirmed by reading Context.cpp: Rml::Context::focus is initialised to the
//     Context's own internal `root` element at CONSTRUCTION time (`focus = root.get();`) and is
//     NEVER reset to nullptr anywhere in the pinned source -- GetFocusElement() therefore never
//     returns nullptr once a Context exists. Rml::Element::Blur() reflects this: when the
//     CURRENTLY focused element is blurred, it calls `parent->Focus()` (re-parents focus UP the
//     tree), not "focus = nullptr". clear_focus() cannot make "nothing focused" happen through
//     RmlUi's public API -- the achievable, and genuinely useful, semantics is "move focus AWAY
//     from wherever it currently is" (e.g. off a game-menu item the host's OWN model no longer
//     considers selected). The tests below use GetFocusElement()->GetId() as the discriminating
//     oracle for exactly that: proving focus moved away from "focusable", not proving it became
//     null (which the pinned RmlUi source makes structurally unreachable).
//
//     Cases (mirrors the guard shape of scroll_sanity.cpp/input_hardening_sanity.cpp):
//       (C0) set_focus/clear_focus before any load() -- both false (no document guard).
//       (C1) set_focus(nullptr) / set_focus("") (AUD-TEC-5 fail-high) / set_focus(unknown id)
//            -- all false, document now loaded.
//       (C2) set_focus("unfocusable") -- false: the element itself is authored `focus: none;`.
//       (A)  set_focus("focusable") -- true, AND the oracle confirms GetFocusElement()'s id is
//            now exactly "focusable" (the "dente" that a stub returning `true` unconditionally
//            would not have).
//       (B)  clear_focus() after (A) -- true, AND the oracle confirms focus moved AWAY from
//            "focusable" (it does NOT and cannot become null, per the Context.cpp finding
//            above).
//       (B2) clear_focus() called AGAIN right after (B) -- still true (idempotent -- see
//            Bootstrap::clear_focus's doc-comment; empirically this exercises Element::Blur()'s
//            own no-parent no-op path once focus has climbed to the Context's rootless root).
//
// PT: focus_sanity -- verifica Engine::set_focus()/clear_focus() (L1.17-FOCUS, consome a
//     semente GAP-4 de docs/embed-integration.md seção 5), o código EXATO para o qual
//     glintfx::UiLayer::set_focus/clear_focus e glintfx::App::set_focus/clear_focus encaminham
//     sem lógica extra (repasse direto, ver ui_layer.cpp/app.cpp) -- então este único teste
//     cobre os dois caminhos públicos de consumo, mesmo raciocínio/precedente do padrão
//     "dirige o Engine diretamente pelo oráculo interno" do document_reload_leak.cpp.
//
//     Dirige o Engine (não o UiLayer) diretamente porque o ORÁCULO necessário aqui --
//     Rml::Context::GetFocusElement() -- só é alcançável de um teste que inclui
//     RmlUi/Core/Context.h + RmlUi/Core/Element.h diretamente; o header público do UiLayer
//     deliberadamente não expõe tipos Rml:: (gate de encapsulamento) e não tem accessor
//     context() para testes se conectarem.
//
//     CORREÇÃO IMPORTANTE vs. suposições ingênuas de tabindex estilo HTML (confirmado lendo o
//     source pinado do RmlUi 6.3, NÃO assumido): Rml::Element::Focus() se protege SÓ pela
//     propriedade RCSS `focus` do elemento (Style::Focus, keyword `auto`|`none`), cujo DEFAULT
//     REGISTRADO NO RmlUi É `auto` (StyleSheetSpecification.cpp) -- ou seja, por padrão TODO
//     elemento (incluindo uma <div> lisa sem estilo) aceita Focus(), A MENOS QUE autorado
//     explicitamente com `focus: none;`. A propriedade RCSS `tab-index` separada (default
//     `none`) controla SÓ a ORDEM de travessia por tecla Tab e não tem NENHUM efeito no
//     sucesso/falha de Focus() -- não há equivalente RmlUi/glintfx da regra implícita do HTML
//     de "só elementos com tabindex são focáveis". focus_scene.rcss portanto autora a guarda
//     REAL diretamente: #focusable fica no default registrado (focus: auto, mais tab-index:
//     auto para espelhar como um host real marcaria um item genuinamente alcançável por Tab),
//     #unfocusable sobrepõe explicitamente `focus: none;`.
//
//     Também confirmado lendo Context.cpp: Rml::Context::focus é inicializado com o próprio
//     elemento interno `root` do Context no momento da CONSTRUÇÃO (`focus = root.get();`) e
//     NUNCA é resetado para nullptr em nenhum lugar do source pinado -- GetFocusElement()
//     portanto nunca retorna nullptr uma vez que um Context exista. Rml::Element::Blur()
//     reflete isso: quando o elemento ATUALMENTE focado é desfocado, ele chama
//     `parent->Focus()` (reparenta o foco PRA CIMA na árvore), não "focus = nullptr".
//     clear_focus() não consegue fazer "nada focado" acontecer pela API pública do RmlUi -- a
//     semântica alcançável, e genuinamente útil, é "mover o foco PRA LONGE de onde está agora"
//     (ex.: tirar de um item de menu de jogo que o MODELO PRÓPRIO do host não considera mais
//     selecionado). Os testes abaixo usam GetFocusElement()->GetId() como o oráculo
//     discriminante para exatamente isso: provar que o foco saiu de "focusable", não provar que
//     ficou nulo (o que o source pinado do RmlUi torna estruturalmente inalcançável).
//
//     Casos (espelha o formato de guarda de scroll_sanity.cpp/input_hardening_sanity.cpp):
//       (C0) set_focus/clear_focus antes de qualquer load() -- ambos false (guarda de
//            documento).
//       (C1) set_focus(nullptr) / set_focus("") (fail-high AUD-TEC-5) / set_focus(id
//            desconhecido) -- todos false, documento já carregado.
//       (C2) set_focus("unfocusable") -- false: o próprio elemento está autorado com `focus:
//            none;`.
//       (A)  set_focus("focusable") -- true, E o oráculo confirma que o id de
//            GetFocusElement() é exatamente "focusable" agora (o "dente" que um stub que
//            sempre retornasse `true` não teria).
//       (B)  clear_focus() após (A) -- true, E o oráculo confirma que o foco saiu de
//            "focusable" (NÃO fica e não pode ficar nulo, pelo achado em Context.cpp acima).
//       (B2) clear_focus() chamado DE NOVO logo após (B) -- ainda true (idempotente -- ver o
//            doc-comment de Bootstrap::clear_focus; empiricamente isto exercita o próprio
//            caminho no-op de "sem pai" do Element::Blur() uma vez que o foco tenha subido até
//            a raiz sem pai do Context).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <cstdio>

// EN: Oracle: true iff something is focused AND its id is exactly `expected_id`.
// PT: Oráculo: true sse algo está focado E seu id é exatamente `expected_id`.
static bool focus_id_is(Rml::Context* ctx, const char* expected_id) {
  Rml::Element* f = ctx->GetFocusElement();
  if (!f) return false;
  return f->GetId() == expected_id;
}

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("focus_host", 200, 150)) { std::puts("FAIL: host create"); return 1; }

  glintfx::SystemClock clock;
  glintfx::Engine engine;
  if (!engine.attach(&clock, 200, 150)) { std::puts("FAIL: engine attach"); return 2; }

  Rml::Context* ctx = engine.context();
  if (!ctx) { std::puts("FAIL: null context after successful attach"); return 3; }

  // ---------------------------------------------------------------------------
  // (C0) Hardening -- no document loaded yet: both methods must fail soft.
  // ---------------------------------------------------------------------------
  if (engine.set_focus("focusable")) {
    std::puts("FAIL: set_focus(\"focusable\") returned true before any load()"); return 4;
  }
  if (engine.clear_focus()) {
    std::puts("FAIL: clear_focus() returned true before any load()"); return 5;
  }

  if (!engine.load("focus_scene.rml")) { std::puts("FAIL: load(focus_scene.rml)"); return 6; }
  engine.update();

  // ---------------------------------------------------------------------------
  // (C1) Hardening -- null id / empty id (AUD-TEC-5) / unknown id, document now loaded.
  // ---------------------------------------------------------------------------
  if (engine.set_focus(nullptr)) { std::puts("FAIL: set_focus(nullptr)"); return 7; }
  if (engine.set_focus("")) { std::puts("FAIL: set_focus(\"\")"); return 8; }
  if (engine.set_focus("does-not-exist")) { std::puts("FAIL: set_focus(unknown id)"); return 9; }

  // ---------------------------------------------------------------------------
  // (C2) Hardening -- an element explicitly authored `focus: none;` must reject Focus().
  // ---------------------------------------------------------------------------
  if (engine.set_focus("unfocusable")) {
    std::puts("FAIL: set_focus(\"unfocusable\") returned true (focus: none; was not honoured)");
    return 10;
  }
  if (focus_id_is(ctx, "unfocusable")) {
    std::puts("FAIL: GetFocusElement() id is \"unfocusable\" after a rejected set_focus -- "
              "the rejected call must not have moved focus at all");
    return 11;
  }

  // ---------------------------------------------------------------------------
  // (A) Happy path -- a genuinely focusable element (registered default `focus: auto;`, plus
  //     `tab-index: auto;` mirroring a real host's markup) must succeed, with the oracle
  //     confirming the id actually moved (the "dente"/tooth: a stub always returning true
  //     would pass the bool check above but fail this one).
  // ---------------------------------------------------------------------------
  if (!engine.set_focus("focusable")) { std::puts("FAIL: set_focus(\"focusable\")"); return 12; }
  if (!focus_id_is(ctx, "focusable")) {
    std::puts("FAIL: GetFocusElement() id is not \"focusable\" after a successful set_focus");
    return 13;
  }

  // ---------------------------------------------------------------------------
  // (B) clear_focus() after a successful set_focus() -- must return true, and the oracle must
  //     confirm focus moved AWAY from "focusable" (per the Context.cpp/Element::Blur() finding
  //     in the file header, it climbs to a PARENT, it does not and cannot become null).
  // ---------------------------------------------------------------------------
  if (!engine.clear_focus()) { std::puts("FAIL: clear_focus() after set_focus"); return 14; }
  if (focus_id_is(ctx, "focusable")) {
    std::puts("FAIL: clear_focus() did not move focus away from \"focusable\"");
    return 15;
  }

  // ---------------------------------------------------------------------------
  // (B2) clear_focus() idempotence -- calling it again right after (B) must still return true
  //      (see Bootstrap::clear_focus's doc-comment for the "no-op is success" rationale).
  // ---------------------------------------------------------------------------
  if (!engine.clear_focus()) { std::puts("FAIL: clear_focus() called again"); return 16; }

  std::puts("focus_sanity: PASS");
  return 0;
}
