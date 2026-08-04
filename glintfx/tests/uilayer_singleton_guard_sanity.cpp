// SPDX-License-Identifier: Apache-2.0
// EN: UILAYER-SINGLETON-GUARD (W26) -- drives the process-wide claim added to Bootstrap::init()/
//     shutdown() this fatia, proving it turns the UILAYER-SINGLETON hazard
//     (docs/embed-integration.md sec. 29: a second UiLayer/App alive at once silently corrupts
//     RmlUi's process-wide globals) into a NAMED, observable failure instead. Four parts, run
//     SEQUENTIALLY in one process (RmlUi's globals are process-wide -- these ARE meant to share
//     state across parts, that is the whole point of this guard):
//       Part 1 -- a second UiLayer constructed while a first is still alive is refused
//                 (`ok() == false`), and the FIRST stays fully functional afterwards (proves the
//                 rejected second never touched a single Rml:: global -- the CAS claim happens
//                 BEFORE Rml::SetSystemInterface/Rml::Initialise are ever called).
//       Part 2 -- destroying the first releases the claim: a THIRD UiLayer, constructed only
//                 after the first goes out of scope, succeeds normally.
//       Part 3 -- white-box, drives glintfx::Bootstrap directly (RenderGl3 + SystemClock, same
//                 pattern capture_nothrow_sanity.cpp's E1/E2 groups already established) with the
//                 src-internal test-only hook `bootstrap_force_create_context_failure_for_test()`
//                 forcing Rml::CreateContext("main", ...) to fail AFTER a successful
//                 Rml::Initialise(). Proves the claim is released on THIS failure path too --
//                 without it, no Bootstrap could EVER init() again, the "process locked out of a
//                 UI forever" failure mode this fatia's brief calls out by name.
//       Part 4 -- the SAME claim rejects a second `App` while a `UiLayer` is alive (not just a
//                 second `UiLayer`), proving the guard is process-wide, not per-type -- the
//                 "consequence declared" scope item (covers App+UiLayer mix too, per the same
//                 mechanism, no special-casing).
//     No mutation harness here (mutation proof for THIS fatia lives in the reviewer's adversarial
//     pass, per house convention) -- this file is the RED/GREEN drive: pre-fix, Parts 1/3/4
//     fail (second instance wrongly succeeds / claim never releases), post-fix all four pass.
// PT: UILAYER-SINGLETON-GUARD (W26) -- dirige o claim de escopo-processo acrescentado ao
//     Bootstrap::init()/shutdown() nesta fatia, provando que ele transforma o perigo do
//     UILAYER-SINGLETON (docs/embed-integration.md seç. 29: um segundo UiLayer/App vivo ao mesmo
//     tempo corrompe em silêncio os globais de processo do RmlUi) numa falha NOMEADA e
//     observável em vez disso. Quatro partes, rodando SEQUENCIALMENTE num único processo (os
//     globais do RmlUi são de escopo-processo -- estas partes DEVEM compartilhar estado entre
//     si, esse é o ponto inteiro deste guard):
//       Parte 1 -- um segundo UiLayer construído enquanto um primeiro ainda está vivo é recusado
//                  (`ok() == false`), e o PRIMEIRO continua plenamente funcional depois (prova
//                  que o segundo recusado nunca tocou em um único global Rml:: -- o claim CAS
//                  acontece ANTES de Rml::SetSystemInterface/Rml::Initialise serem sequer
//                  chamados).
//       Parte 2 -- destruir o primeiro libera o claim: um TERCEIRO UiLayer, construído só depois
//                  do primeiro sair de escopo, tem sucesso normal.
//       Parte 3 -- white-box, dirige glintfx::Bootstrap direto (RenderGl3 + SystemClock, mesmo
//                  padrão que os grupos E1/E2 de capture_nothrow_sanity.cpp já estabeleceram) com
//                  o hook src-interno só-de-teste
//                  `bootstrap_force_create_context_failure_for_test()` forçando o
//                  Rml::CreateContext("main", ...) a falhar DEPOIS de um Rml::Initialise() bem-
//                  sucedido. Prova que o claim é liberado neste caminho de falha também -- sem
//                  isso, nenhum Bootstrap poderia JAMAIS dar init() de novo, o modo de falha
//                  "processo travado sem UI pra sempre" que o brief desta fatia nomeia.
//       Parte 4 -- o MESMO claim recusa um segundo `App` enquanto um `UiLayer` está vivo (não só
//                  um segundo `UiLayer`), provando que o guard é de escopo-processo, não
//                  por-tipo -- o item de escopo "consequência declarada" (cobre a mistura
//                  App+UiLayer também, pelo mesmo mecanismo, sem tratamento especial).
//     Sem harness de mutação aqui (a prova de mutação desta fatia mora no passe adversarial do
//     revisor, convenção da casa) -- este arquivo é o drive RED/GREEN: pré-fix, as Partes 1/3/4
//     falham (segundo instância indevidamente tem sucesso / claim nunca libera), pós-fix as
//     quatro passam.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/bootstrap.hpp"
#include "../src/render_gl3.hpp"
#include "../src/system_clock.hpp"
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>

using namespace glintfx;

namespace {

int g_failures = 0;

void expect(const char* what, bool condition) {
  if (!condition) {
    ++g_failures;
    std::fprintf(stderr, "uilayer_singleton_guard_sanity FAIL: %s\n", what);
  }
}

} // namespace

int main() {
  // EN: ONE host window/GL context for Parts 1-3 -- every UiLayer/Bootstrap below attaches to
  //     whatever context is current, none owns a window of its own (embed mode). Part 4
  //     constructs a real `App`, which DOES own (and makes current) its own window/context --
  //     kept last so nothing after it depends on `host`'s context still being current.
  // PT: UM host window/contexto GL para as Partes 1-3 -- todo UiLayer/Bootstrap abaixo se anexa
  //     ao contexto que estiver corrente, nenhum é dono de janela própria (modo embed). A Parte 4
  //     constrói um `App` de verdade, que POSSUI (e torna corrente) a própria janela/contexto --
  //     mantida por último para que nada depois dela dependa do contexto do `host` continuar
  //     corrente.
  WindowGlfw host;
  if (!host.create("uilayer_singleton_guard_sanity", 320, 240)) {
    std::fprintf(stderr, "uilayer_singleton_guard_sanity SETUP FAIL: host window create failed\n");
    return 1;
  }

  // ---------------------------------------------------------------------------------------
  // Part 1 -- second UiLayer refused while first is alive; first stays fully functional.
  // ---------------------------------------------------------------------------------------
  {
    UiLayer first({.logical_width = 320, .logical_height = 240});
    expect("[P1] first.ok()", first.ok());

    UiLayer second({.logical_width = 320, .logical_height = 240});
    expect("[P1] second.ok() must be false (guard refuses a live-while-live second instance)",
           !second.ok());

    // EN: The rejected `second` must not have touched ANY global -- prove `first` is still
    //     fully usable: ok() unchanged, update()/render() do not crash, and load() still
    //     reaches a live Rml::Context (proves Bootstrap::context() on `first` was never nulled
    //     out by `second`'s failed attempt).
    // PT: O `second` recusado não pode ter tocado NENHUM global -- prova que `first` continua
    //     plenamente usável: ok() inalterado, update()/render() não crasham, e load() ainda
    //     alcança um Rml::Context vivo (prova que o Bootstrap::context() de `first` nunca foi
    //     zerado pela tentativa falha de `second`).
    expect("[P1] first.ok() still true after rejected second", first.ok());
    first.update();
    first.render();
    expect("[P1] first.load(min.rml) still works after rejected second",
           first.load("tests/min.rml"));
  }
  // EN: `second` (moved-from-like: never claimed, Bootstrap::impl_ stayed null) destructs
  //     first (reverse declaration order), a no-op release; `first` destructs next, releasing
  //     the real claim.
  // PT: `second` (como movido-de: nunca reivindicou, Bootstrap::impl_ ficou nulo) destrói
  //     primeiro (ordem reversa de declaração), uma liberação no-op; `first` destrói em
  //     seguida, liberando o claim de verdade.

  // ---------------------------------------------------------------------------------------
  // Part 2 -- claim released on destruction: a third instance, built only after the first is
  // gone, succeeds normally.
  // ---------------------------------------------------------------------------------------
  {
    UiLayer third({.logical_width = 320, .logical_height = 240});
    expect("[P2] third.ok() after first UiLayer was destroyed", third.ok());
  }

  // ---------------------------------------------------------------------------------------
  // Part 3 -- white-box: forced Rml::CreateContext(...) failure releases the claim too (else
  // the process could never init() a Bootstrap again).
  // ---------------------------------------------------------------------------------------
  {
    SystemClock clock;
    RenderGl3 render;
    if (!render.init()) {
      std::fprintf(stderr, "uilayer_singleton_guard_sanity SETUP FAIL: RenderGl3::init() failed\n");
      return 1;
    }

    bootstrap_force_create_context_failure_for_test() = true;
    Bootstrap forced_fail;
    expect("[P3] init() returns false when CreateContext is forced to fail",
           !forced_fail.init(&clock, render, 320, 240));
    bootstrap_force_create_context_failure_for_test() = false;

    // EN: If the claim had leaked in the forced-failure branch above, THIS init() would be
    //     refused too (same symptom as Part 1) -- succeeding here is the proof the release
    //     happened.
    // PT: Se o claim tivesse vazado no ramo de falha forçada acima, ESTE init() também seria
    //     recusado (mesmo sintoma da Parte 1) -- ter sucesso aqui é a prova de que a liberação
    //     aconteceu.
    Bootstrap recovers;
    expect("[P3] claim released after forced CreateContext failure -> a fresh init() succeeds",
           recovers.init(&clock, render, 320, 240));
    recovers.shutdown();
  }

  // ---------------------------------------------------------------------------------------
  // Part 4 -- the SAME process-wide claim rejects a second `App` while a `UiLayer` is alive
  // (the "consequence declared" scope: not just a second UiLayer). Last part: App::App() owns
  // and makes current its OWN window/context, so nothing below may rely on `host`'s context.
  // ---------------------------------------------------------------------------------------
  {
    UiLayer layer({.logical_width = 320, .logical_height = 240});
    expect("[P4] layer.ok()", layer.ok());

    App app({.title = "uilayer_singleton_guard_sanity-app", .width = 320, .height = 240});
    expect(
        "[P4] a second App while a UiLayer is alive must be refused (ok()==false) -- "
        "same process-wide claim, no per-type special-casing",
        !app.ok());
  }

  if (g_failures != 0) {
    std::fprintf(stderr, "uilayer_singleton_guard_sanity: %d failure(s)\n", g_failures);
    return 1;
  }
  std::puts("uilayer_singleton_guard_sanity: OK");
  return 0;
}
