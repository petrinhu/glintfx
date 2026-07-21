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
//     WindowGlfw::set_mode()'s `current == mode` early-return).
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
//     early-return `current == mode` do WindowGlfw::set_mode()).
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
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
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
  //     must return true (D7).
  // PT: Repede o modo CORRENTE (Windowed, do último passo acima) -- no-op documentado, precisa
  //     retornar true (D7).
  // ---------------------------------------------------------------------------
  if (!app.set_window_mode(glintfx::WindowMode::Windowed)) {
    std::puts("FAIL: re-requesting the current mode (Windowed) returned false");
    return 7;
  }
  if (app.window_mode() != glintfx::WindowMode::Windowed) {
    std::puts("FAIL: window_mode() changed after a same-mode re-request");
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

  std::puts("app_window_mode_transitions_smoke OK");
  return 0;
}
