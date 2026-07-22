// SPDX-License-Identifier: MPL-2.0
// EN: App runtime window-mode TRANSITIONS smoke (A4-WINMODES, v0.16.0). One App instance
//     (N3 -- construction happens ONCE, unlike app_window_mode_smoke.cpp's per-mode-process
//     idiom), cycling App::set_window_mode() through every mode the D1 runtime-switch decision
//     exists to support:
//       Windowed -> Maximized -> Windowed -> FullscreenDesktop -> Windowed ->
//       FullscreenExclusive -> Maximized -> Windowed
//     which exercises every pairwise transition class the D7/D8 machinery (window_glfw.cpp)
//     has a distinct code path for: Windowed<->Maximized (glfwRestoreWindow/
//     glfwMaximizeWindow), Windowed<->Fullscreen (glfwSetWindowMonitor with/without a
//     monitor), and Fullscreen->Maximized (D8's MANDATORY restore-windowed-first-then-maximize
//     order -- the one ordering GLFW does not support directly).
//
//     Between EVERY transition: poll_events()+update()+render(), then get_window_size() must
//     stay positive on both axes (proves no frame renders at a stale/zero framebuffer size --
//     the AUD-PUB-1 sync_viewport() machinery re-flows the layout for whichever size the mode
//     change left the window at, with ZERO new plumbing per the plan's "Resize: livre"
//     summary).
//
//     RE-REQUEST NO-OP: after settling into Windowed the first time, re-requesting Windowed
//     again must return true without touching the window (D7's own no-op guard,
//     WindowGlfw::set_mode()'s `dispatched_mode_ == req_mode` early-return).
//
//     HARDENING (same binary, no extra App instance -- N3): set_window_mode(static_cast<
//     WindowMode>(99)) with an out-of-range enum must return false AND leave window_mode()
//     completely unchanged (D5's fail-high no-op contract) -- captures the live mode
//     immediately before and after the call and diffs them, rather than asserting a fixed
//     expected value, so this check is valid no matter where in the cycle it runs.
//
//     NOT covered here (per the plan's own scope note, section 2.3 item 2): a SEPARATE App
//     constructed with a garbage AppConfig::window_mode is app_window_mode_smoke.cpp's
//     "invalid_config" run (N3 forbids a 2nd App in this process) -- this file only hardens
//     the RUNTIME setter, not the ctor-time field.
//
//     REVIEW REGRESSIONS (2026-07-21):
//
//     MAJOR #1 -- back-to-back Maximized->Windowed with ZERO poll/render between the two
//     set_window_mode() calls (the exact pattern the adversarial review reproduced 3/3 on a
//     REAL KDE/Wayland session: the second call used to read the stale, not-yet-WM-confirmed
//     GLFW_MAXIMIZED attrib, mistake it for "already Windowed", and silently no-op --
//     stranding the window maximized forever once the WM's confirmation eventually landed).
//     Fixed by tracking WindowGlfw::dispatched_mode_ (our own synchronous ledger of the last
//     mode WE told the window system to become) instead of re-deriving from the live,
//     WM-confirmed mode() query for this internal decision -- see window_glfw.hpp's
//     dispatched_mode_ doc-comment for the full account. HONEST LIMIT: under Xvfb there is no
//     window manager at all, so GLFW_MAXIMIZED never gets asynchronously confirmed by anyone --
//     the specific race this section targets is NOT reproducible headless (there is nothing to
//     race against). This block still asserts the documented API CONTRACT (both calls succeed,
//     the window settles sane) and guards a future regression that reintroduces a live-mode()
//     read for the internal decision; the definitive proof is the manual run of THIS test
//     binary on a real display, 3/3, required by the review before merge.
//
//     MINOR (d) -- geometry-restore coverage gap found by the review's mutation-test: removing
//     the SIZE half of save_windowed_geometry()'s D7 snapshot (window_glfw.cpp), keeping only
//     position, killed no test. Code inspection confirms save_windowed_geometry() DOES call
//     glfwGetWindowSize() today -- this was a coverage gap, not a live bug -- closed by forcing
//     a REAL OS-level resize (glfwSetWindowSize(), same test-only-GLFW-dependency idiom as
//     app_resize_smoke.cpp) to a size different from both the construction size and any
//     window-mode default, then cycling away from Windowed and back: the restored size must be
//     the RESIZED one, which only holds if save_windowed_geometry() captured the LIVE size at
//     the moment of the transition request, not a stale construction-time value.
//
//     ADDITIONAL FINDING on the 1st real run (2026-07-21, the líder's KDE/Wayland session,
//     BEFORE the fix below): the dispatched_mode_ fix (MAJOR #1) closed the premature no-op,
//     but exposed a second dimension of the SAME asynchronous lag -- the LIVE window_mode()
//     query itself (D4, glfwGetWindowAttrib(GLFW_MAXIMIZED)) can take MORE than a single
//     poll+update+render cycle to converge on a real WM (KWin), even after settle_and_check()
//     already ran once. The original re-request no-op check (`if (app.window_mode() !=
//     Windowed)` right after the 7-step cycle) failed with "window_mode() changed after a
//     same-mode re-request" -- NOT because dispatched_mode_ was wrong (set_window_mode(Windowed)
//     correctly returned true, via the no-op), but because the LIVE query itself had not
//     converged yet. Fixed with settle_until_mode() below: a BOUNDED wait (glfwWaitEventsTimeout,
//     GLFW's own idiomatic primitive for "wait for an async window-system event with a
//     deadline", NOT a blind `sleep` -- house style's "polling com backoff") until window_mode()
//     converges or attempts run out, used only where the test asserts window_mode() directly
//     against a fixed target (the 7-step loop itself only checks get_window_size()>0, which
//     already converges in 1 frame even on the real WM -- it did not need the helper).
//
// PT: Smoke de TRANSIÇÕES de modo de janela em runtime do App (A4-WINMODES, v0.16.0). Uma
//     única instância de App (N3 -- construção acontece UMA VEZ, diferente do idioma
//     um-processo-por-modo de app_window_mode_smoke.cpp), ciclando App::set_window_mode() por
//     todo modo que a decisão de runtime-switch D1 existe pra suportar:
//       Windowed -> Maximized -> Windowed -> FullscreenDesktop -> Windowed ->
//       FullscreenExclusive -> Maximized -> Windowed
//     o que exercita toda classe de transição par-a-par pra qual a maquinaria D7/D8
//     (window_glfw.cpp) tem um caminho de código distinto: Windowed<->Maximized
//     (glfwRestoreWindow/glfwMaximizeWindow), Windowed<->Fullscreen (glfwSetWindowMonitor com/
//     sem monitor), e Fullscreen->Maximized (a ordem OBRIGATÓRIA do D8 de restaurar-windowed-
//     primeiro-depois-maximizar -- a única ordem que o GLFW não suporta diretamente).
//
//     Entre TODA transição: poll_events()+update()+render(), depois get_window_size() precisa
//     continuar positivo nos dois eixos (prova que nenhum frame renderiza num tamanho de
//     framebuffer velho/zero -- a maquinaria sync_viewport() do AUD-PUB-1 refaz o layout pra
//     qualquer tamanho que a mudança de modo tenha deixado a janela, com ZERO plumbing novo
//     conforme o resumo "Resize: livre" do plano).
//
//     NO-OP DE REPEDIDO: depois de assentar em Windowed pela primeira vez, repedir Windowed de
//     novo precisa retornar true sem tocar a janela (a própria guarda no-op do D7,
//     early-return `dispatched_mode_ == req_mode` do WindowGlfw::set_mode()).
//
//     HARDENING (mesmo binário, sem instância extra de App -- N3): set_window_mode(
//     static_cast<WindowMode>(99)) com enum fora de faixa precisa retornar false E deixar
//     window_mode() completamente inalterado (contrato no-op fail-high do D5) -- captura o
//     modo vivo imediatamente antes e depois da chamada e compara, em vez de afirmar um valor
//     esperado fixo, então esta checagem vale não importa onde no ciclo ela roda.
//
//     NÃO coberto aqui (conforme a própria nota de escopo do plano, seção 2.3 item 2): um App
//     SEPARADO construído com um AppConfig::window_mode lixo é a corrida "invalid_config" de
//     app_window_mode_smoke.cpp (o N3 proíbe um 2º App neste processo) -- este arquivo só
//     endurece o setter de RUNTIME, não o campo em tempo de ctor.
//
//     REGRESSÕES DO REVIEW (2026-07-21):
//
//     MAJOR #1 -- Maximized->Windowed nas costas uma da outra, ZERO poll/render entre as duas
//     chamadas set_window_mode() (o padrão exato que o review adversarial reproduziu 3/3 numa
//     sessão KDE/Wayland REAL: a segunda chamada lia o attrib GLFW_MAXIMIZED desatualizado,
//     ainda não confirmado pelo WM, confundia com "já Windowed", e virava no-op silencioso --
//     travando a janela maximizada pra sempre assim que a confirmação do WM finalmente
//     chegasse). Corrigido rastreando WindowGlfw::dispatched_mode_ (nosso próprio livro-razão
//     síncrono do último modo que PEDIMOS ao sistema de janelas) em vez de re-derivar da query
//     mode() viva, confirmada pelo WM, pra esta decisão interna -- ver o doc-comment de
//     dispatched_mode_ em window_glfw.hpp pro relato completo. LIMITE HONESTO: sob Xvfb não há
//     window manager nenhum, então o GLFW_MAXIMIZED nunca é confirmado assincronamente por
//     ninguém -- a race específica que esta seção mira NÃO é reproduzível headless (não há nada
//     pra competir contra). Este bloco ainda assim afirma o CONTRATO de API documentado (as
//     duas chamadas têm sucesso, a janela assenta são) e protege contra uma regressão futura
//     que reintroduza uma leitura de mode() vivo pra esta decisão interna; a prova definitiva é
//     a corrida manual deste binário de teste num display real, 3/3, exigida pelo review antes
//     do merge.
//
//     MINOR (d) -- lacuna de cobertura de restauração de geometria achada pelo mutation-test do
//     review: remover a metade TAMANHO do snapshot D7 de save_windowed_geometry()
//     (window_glfw.cpp), guardando só posição, não matou nenhum teste. Inspeção de código
//     confirma que save_windowed_geometry() JÁ chama glfwGetWindowSize() hoje -- era uma lacuna
//     de cobertura, não um bug vivo -- fechada forçando um resize REAL no nível do SO
//     (glfwSetWindowSize(), mesmo idioma de dependência-de-GLFW-só-de-teste do
//     app_resize_smoke.cpp) pra um tamanho diferente tanto do tamanho de construção quanto de
//     qualquer default de modo de janela, depois ciclando pra fora de Windowed e de volta: o
//     tamanho restaurado precisa ser o REDIMENSIONADO, o que só vale se save_windowed_geometry()
//     capturou o tamanho VIVO no momento do pedido de transição, não um valor de tempo-de-
//     construção obsoleto.
//
//     ACHADO ADICIONAL na 1ª corrida real (2026-07-21, sessão KDE/Wayland do líder, ANTES do
//     fix abaixo): o fix do dispatched_mode_ (MAJOR #1) corrigiu o no-op prematuro, mas expôs
//     uma segunda dimensão da MESMA lentidão assíncrona -- a query VIVA window_mode() (D4, o
//     próprio glfwGetWindowAttrib(GLFW_MAXIMIZED)) pode levar MAIS de um único ciclo
//     poll+update+render pra convergir num WM real (KWin), mesmo já tendo assentado
//     (settle_and_check) uma vez. A checagem de no-op-de-repedido original (`if
//     (app.window_mode() != Windowed)` logo após o ciclo de 7 passos) falhava com
//     "window_mode() changed after a same-mode re-request" -- não porque dispatched_mode_
//     estivesse errado (a chamada set_window_mode(Windowed) retornou true, como devia,
//     via no-op), mas porque a PRÓPRIA query viva ainda não tinha convergido. Corrigido com
//     settle_until_mode() abaixo: espera LIMITADA (glfwWaitEventsTimeout, o primitivo
//     idiomático do GLFW pra "esperar um evento assíncrono do sistema de janelas com prazo",
//     NÃO um `sleep` cego -- "polling com backoff" da casa) até window_mode() convergir ou
//     esgotar as tentativas, usada só onde o teste afirma window_mode() diretamente contra um
//     alvo fixo (o loop de 7 passos em si só verifica get_window_size()>0, que já converge em
//     1 frame mesmo no WM real -- não precisou do helper).
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <GLFW/glfw3.h>  // EN: test-only -- glfwGetCurrentContext/glfwSetWindowSize for the MINOR (d) OS-level resize (same idiom as app_resize_smoke.cpp). PT: só-de-teste -- glfwGetCurrentContext/glfwSetWindowSize pro resize MINOR (d) no nível do SO (mesmo idioma de app_resize_smoke.cpp).
#include <cstdio>

namespace {

// EN: poll+update+render, then assert get_window_size() is sane. `label` identifies the
//     transition in FAIL output for a quick repro.
// PT: poll+update+render, depois afirma que get_window_size() é são. `label` identifica a
//     transição na saída de FAIL pra um repro rápido.
bool settle_and_check(glintfx::App& app, const char* label) {
  app.poll_events();
  app.update();
  app.render();
  int w = 0, h = 0;
  app.get_window_size(w, h);
  if (w <= 0 || h <= 0) {
    std::printf("FAIL: get_window_size() non-positive %dx%d after %s\n", w, h, label);
    return false;
  }
  std::printf("app_window_mode_transitions_smoke: after %s -> %dx%d\n", label, w, h);
  return true;
}

// EN: Bounded wait until the LIVE window_mode() query converges to `expected`, added for the
//     real-WM finding documented in the file header above (settle_and_check()'s single frame is
//     enough for get_window_size(), but NOT always enough for the WM to confirm a
//     maximize/restore over the compositor protocol). glfwWaitEventsTimeout() is GLFW's own
//     blocking-wait-for-window-system-events primitive with a deadline -- the correct tool for
//     "wait for an asynchronous external confirmation", not a blind fixed sleep (house
//     anti-pattern). Bounded by kMaxAttempts so a genuinely stuck/broken transition still fails
//     the test in finite time instead of hanging.
// PT: Espera limitada até a query VIVA window_mode() convergir pra `expected`, adicionada pelo
//     achado no WM real documentado no cabeçalho do arquivo acima (o frame único do
//     settle_and_check() basta pro get_window_size(), mas NEM SEMPRE basta pro WM confirmar um
//     maximize/restore pelo protocolo do compositor). glfwWaitEventsTimeout() é o próprio
//     primitivo bloqueante-de-espera-por-eventos-do-sistema-de-janelas do GLFW com prazo -- a
//     ferramenta correta pra "esperar uma confirmação externa assíncrona", não um sleep fixo
//     cego (anti-pattern da casa). Limitada por kMaxAttempts pra que uma transição
//     genuinamente travada/quebrada ainda falhe o teste em tempo finito, em vez de travar.
bool settle_until_mode(glintfx::App& app, glintfx::WindowMode expected, const char* label) {
  constexpr int kMaxAttempts = 40;  // EN: ~2s worst case at 50ms/attempt. PT: ~2s pior caso a 50ms/tentativa.
  for (int attempt = 0; attempt < kMaxAttempts && app.window_mode() != expected; ++attempt) {
    glfwWaitEventsTimeout(0.05);
    app.update();
    app.render();
  }
  int w = 0, h = 0;
  app.get_window_size(w, h);
  if (w <= 0 || h <= 0) {
    std::printf("FAIL: get_window_size() non-positive %dx%d after %s\n", w, h, label);
    return false;
  }
  if (app.window_mode() != expected) {
    std::printf("FAIL: window_mode() did not converge to %d after %s (still %d after %d "
                "poll attempts)\n", static_cast<int>(expected), label,
                static_cast<int>(app.window_mode()), kMaxAttempts);
    return false;
  }
  std::printf("app_window_mode_transitions_smoke: after %s -> %dx%d, mode converged\n",
              label, w, h);
  return true;
}

} // namespace

int main() {
  glintfx::App app({ .title = "app_window_mode_transitions_smoke", .width = 400, .height = 300 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); min.rml is
  //     copied unconditionally at the top of tests/CMakeLists.txt.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo); min.rml
  //     é copiado incondicionalmente no topo do tests/CMakeLists.txt.
  if (!app.load("tests/min.rml")) { std::puts("FAIL: load"); return 2; }

  if (app.window_mode() != glintfx::WindowMode::Windowed) {
    std::puts("FAIL: initial live window_mode() is not Windowed");
    return 3;
  }
  if (!settle_and_check(app, "construction (Windowed)")) return 4;

  // ---------------------------------------------------------------------------
  // EN: Full transition cycle -- see file header for why this exact sequence covers every
  //     pairwise transition class D7/D8 implements.
  // PT: Ciclo completo de transições -- ver cabeçalho do arquivo pra por que esta sequência
  //     exata cobre toda classe de transição par-a-par que D7/D8 implementa.
  // ---------------------------------------------------------------------------
  struct Step { glintfx::WindowMode mode; const char* label; };
  const Step steps[] = {
    { glintfx::WindowMode::Maximized,            "Windowed->Maximized" },
    { glintfx::WindowMode::Windowed,             "Maximized->Windowed" },
    { glintfx::WindowMode::FullscreenDesktop,    "Windowed->FullscreenDesktop" },
    { glintfx::WindowMode::Windowed,             "FullscreenDesktop->Windowed" },
    { glintfx::WindowMode::FullscreenExclusive,  "Windowed->FullscreenExclusive" },
    { glintfx::WindowMode::Maximized,            "FullscreenExclusive->Maximized (D8 order)" },
    { glintfx::WindowMode::Windowed,             "Maximized->Windowed (final)" },
  };
  int step_no = 0;
  for (const Step& step : steps) {
    ++step_no;
    if (!app.set_window_mode(step.mode)) {
      std::printf("FAIL: set_window_mode() returned false for step %d (%s)\n",
                  step_no, step.label);
      return 5;
    }
    if (!settle_and_check(app, step.label)) return 6;
  }

  // ---------------------------------------------------------------------------
  // EN: Re-request the CURRENT mode (Windowed, from the last step above) -- documented no-op,
  //     must return true (D7). Uses settle_until_mode() (not a bare window_mode() read) because
  //     the LIVE query itself can still be catching up to the LAST cycle step's
  //     Maximized->Windowed transition on a real WM (see the file header's "additional finding").
  // PT: Repede o modo CORRENTE (Windowed, do último passo acima) -- no-op documentado, precisa
  //     retornar true (D7). Usa settle_until_mode() (não uma leitura nua de window_mode())
  //     porque a query VIVA em si ainda pode estar alcançando a transição Maximized->Windowed
  //     do último passo do ciclo num WM real (ver o "achado adicional" no cabeçalho do arquivo).
  // ---------------------------------------------------------------------------
  if (!app.set_window_mode(glintfx::WindowMode::Windowed)) {
    std::puts("FAIL: re-requesting the current mode (Windowed) returned false");
    return 7;
  }
  if (!settle_until_mode(app, glintfx::WindowMode::Windowed, "same-mode re-request (Windowed)")) {
    return 8;
  }

  // ---------------------------------------------------------------------------
  // EN: Hardening -- out-of-range enum (D5). Must return false and leave the live mode
  //     completely untouched; compares before/after rather than a hardcoded expectation so the
  //     assertion is valid regardless of where in the cycle it runs.
  // PT: Hardening -- enum fora de faixa (D5). Precisa retornar false e deixar o modo vivo
  //     completamente intocado; compara antes/depois em vez de uma expectativa hardcoded, então
  //     a asserção vale independente de onde no ciclo ela roda.
  // ---------------------------------------------------------------------------
  const glintfx::WindowMode before_hostile = app.window_mode();
  if (app.set_window_mode(static_cast<glintfx::WindowMode>(99))) {
    std::puts("FAIL: set_window_mode(invalid enum 99) returned true (expected false, D5)");
    return 9;
  }
  const glintfx::WindowMode after_hostile = app.window_mode();
  if (after_hostile != before_hostile) {
    std::printf("FAIL: invalid set_window_mode() call mutated live mode (%d -> %d)\n",
                static_cast<int>(before_hostile), static_cast<int>(after_hostile));
    return 10;
  }
  // EN: A second, differently-out-of-range value -- not a magic-single-value fluke.
  // PT: Um segundo valor, fora de faixa de forma diferente -- não é acaso de um único valor mágico.
  if (app.set_window_mode(static_cast<glintfx::WindowMode>(255))) {
    std::puts("FAIL: set_window_mode(invalid enum 255) returned true (expected false, D5)");
    return 11;
  }
  if (app.window_mode() != before_hostile) {
    std::puts("FAIL: 2nd invalid set_window_mode() call mutated live mode");
    return 12;
  }
  if (!settle_and_check(app, "post-hardening")) return 13;

  // ---------------------------------------------------------------------------
  // EN: MAJOR #1 regression (review, 2026-07-21) -- back-to-back Maximized->Windowed, ZERO
  //     poll_events()/update()/render() between the two set_window_mode() calls. See the file
  //     header for the full account and the HONEST LIMIT this section has under Xvfb (no WM to
  //     race against here -- the definitive proof is running THIS binary on a real display,
  //     3/3, per the review's own instruction). What this DOES prove even headless: both calls
  //     succeed, and neither silently no-ops the other -- a future regression that reintroduces
  //     a live mode() read for the internal no-op decision would, on THIS host, most likely
  //     still pass (the race needs a live WM) but the assertions below are still the correct,
  //     documented contract to enforce regardless of platform.
  // PT: Regressão MAJOR #1 (review, 2026-07-21) -- Maximized->Windowed nas costas uma da outra,
  //     ZERO poll_events()/update()/render() entre as duas chamadas set_window_mode(). Ver o
  //     cabeçalho do arquivo pro relato completo e o LIMITE HONESTO que esta seção tem sob Xvfb
  //     (sem WM pra competir aqui -- a prova definitiva é rodar ESTE binário num display real,
  //     3/3, conforme a própria instrução do review). O que isto PROVA mesmo headless: as duas
  //     chamadas têm sucesso, e nenhuma vira no-op silencioso da outra -- uma regressão futura
  //     que reintroduza uma leitura de mode() vivo pra decisão interna de no-op provavelmente
  //     AINDA passaria NESTE host (a race precisa de um WM vivo), mas as asserções abaixo ainda
  //     são o contrato correto e documentado a exigir, independente de plataforma.
  // ---------------------------------------------------------------------------
  if (!app.set_window_mode(glintfx::WindowMode::Maximized)) {
    std::puts("FAIL: set_window_mode(Maximized) (MAJOR #1 setup) returned false");
    return 14;
  }
  // EN: NO poll_events()/update()/render() here -- this is the point. The next call must not
  //     read a stale confirmation of the request just above.
  // PT: NENHUM poll_events()/update()/render() aqui -- este é o ponto. A próxima chamada não
  //     pode ler uma confirmação desatualizada do pedido logo acima.
  if (!app.set_window_mode(glintfx::WindowMode::Windowed)) {
    std::puts("FAIL: set_window_mode(Windowed) immediately after Maximized (MAJOR #1) "
              "returned false");
    return 15;
  }
  // EN: settle_until_mode() (not settle_and_check() + a bare read) -- same real-WM convergence
  //     reasoning as the re-request check above; see the file header's "additional finding".
  // PT: settle_until_mode() (não settle_and_check() + uma leitura nua) -- mesmo raciocínio de
  //     convergência em WM real do check de repedido acima; ver o "achado adicional" no
  //     cabeçalho do arquivo.
  if (!settle_until_mode(app, glintfx::WindowMode::Windowed,
                          "back-to-back Maximized->Windowed (MAJOR #1)")) {
    return 17;
  }

  // ---------------------------------------------------------------------------
  // EN: MINOR (d) regression (review, 2026-07-21) -- geometry-restore must preserve a SIZE the
  //     game set itself (via a real OS-level resize) while Windowed, not just the construction
  //     size. See the file header for the full account. App does not expose its GLFWwindow*
  //     (no third-party type crosses the public boundary) -- same test-only-GLFW-dependency
  //     fixture app_resize_smoke.cpp uses: glfwGetCurrentContext() deterministically returns
  //     this App's own window handle here (WindowGlfw::create() called glfwMakeContextCurrent()
  //     during construction and nothing else changes the current context; N3 guarantees a
  //     single App per process).
  // PT: Regressão MINOR (d) (review, 2026-07-21) -- a restauração de geometria precisa
  //     preservar um TAMANHO que o próprio jogo definiu (via um resize real no nível do SO)
  //     enquanto Windowed, não só o tamanho de construção. Ver o cabeçalho do arquivo pro
  //     relato completo. O App não expõe seu GLFWwindow* (nenhum tipo de terceiro cruza a
  //     fronteira pública) -- mesmo fixture de dependência-de-GLFW-só-de-teste que
  //     app_resize_smoke.cpp usa: glfwGetCurrentContext() retorna deterministicamente o handle
  //     de janela do próprio App aqui (WindowGlfw::create() chamou glfwMakeContextCurrent()
  //     durante a construção e nada mais troca o contexto corrente; o N3 garante um único App
  //     por processo).
  // ---------------------------------------------------------------------------
  GLFWwindow* win = glfwGetCurrentContext();
  if (!win) { std::puts("FAIL: glfwGetCurrentContext() returned null (MINOR (d) setup)"); return 18; }

  // EN: 350x220 -- deliberately different from BOTH the construction size (400x300) and any
  //     window-mode default, so a stale/construction-time restore is unambiguously caught.
  // PT: 350x220 -- deliberadamente diferente TANTO do tamanho de construção (400x300) quanto de
  //     qualquer default de modo de janela, então uma restauração obsoleta/de-tempo-de-
  //     construção é capturada sem ambiguidade.
  glfwSetWindowSize(win, 350, 220);
  // EN: Two-frame settle, same convention app_resize_smoke.cpp documents (Step 2's comment
  //     there): a single poll_events() after glfwSetWindowSize() is not always enough.
  // PT: Assentamento em dois frames, mesma convenção que app_resize_smoke.cpp documenta
  //     (comentário do Passo 2 lá): um único poll_events() após glfwSetWindowSize() nem sempre
  //     basta.
  if (!settle_and_check(app, "OS-level resize to 350x220 (MINOR (d) setup)")) return 19;
  if (!settle_and_check(app, "OS-level resize to 350x220, 2nd frame (MINOR (d) setup)")) return 20;

  int resized_w = 0, resized_h = 0;
  app.get_window_size(resized_w, resized_h);
  if (resized_w != 350 || resized_h != 220) {
    std::printf("FAIL: OS-level resize did not take (%dx%d != 350x220) -- cannot proceed with "
                "the MINOR (d) geometry-restore regression\n", resized_w, resized_h);
    return 21;
  }

  if (!app.set_window_mode(glintfx::WindowMode::FullscreenDesktop)) {
    std::puts("FAIL: set_window_mode(FullscreenDesktop) after resize (MINOR (d)) returned false");
    return 22;
  }
  if (!settle_and_check(app, "resized Windowed->FullscreenDesktop (MINOR (d))")) return 23;

  if (!app.set_window_mode(glintfx::WindowMode::Windowed)) {
    std::puts("FAIL: set_window_mode(Windowed) restore after resize+fullscreen (MINOR (d)) "
              "returned false");
    return 24;
  }
  if (!settle_and_check(app, "FullscreenDesktop->Windowed, geometry restore (MINOR (d))")) return 25;

  int restored_w = 0, restored_h = 0;
  app.get_window_size(restored_w, restored_h);
  if (restored_w != 350 || restored_h != 220) {
    std::printf("FAIL: restored size %dx%d != resized size 350x220 -- save_windowed_geometry() "
                "did not capture the SIZE half of the D7 snapshot at the moment of the "
                "transition (MINOR (d) regression)\n", restored_w, restored_h);
    return 26;
  }

  std::puts("app_window_mode_transitions_smoke OK");
  return 0;
}
