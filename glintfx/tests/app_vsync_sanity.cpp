// SPDX-License-Identifier: Apache-2.0
// EN: app_vsync_sanity -- contract test for `WM-VSYNC` (window-modes finish wave,
//     consumer-driven, líder-approved scope 2026-07-23): `App::set_swap_interval(int)`/
//     `App::set_vsync(bool)` (sugar over the former) and `App::get_monitor_refresh_hz()`.
//
//     HEADLESS HONESTY (declared, not faked -- see the team's own review policy on downgrading
//     instead of inventing coverage): under Xvfb/Mesa-llvmpipe there is no real display, no
//     real compositor pacing, and no GLFW query that echoes back the swap interval a driver
//     actually applied -- `glfwSwapInterval` is fire-and-forget from GLFW's own API surface.
//     This file therefore proves the CONTRACT (guards, idempotency, forwarding), never the
//     REAL vsync EFFECT (no frame-timing assertion anywhere in this file -- that would be
//     flaky by construction under a driver with no real refresh to pace against). Two
//     specific properties are UNVERIFIABLE here and are named rather than silently skipped:
//       - Whether the platform driver/compositor actually HONOURS a requested interval (D9's
//         own "VSYNC HONESTY" note in app.hpp documents the same class of limitation for the
//         pre-existing render()/run() pacing).
//       - `App::set_vsync(bool)`'s specific true->1 / false->0 mapping cannot be told apart by
//         its OBSERVABLE effect (no query exists); only that BOTH calls succeed (reach a
//         guard-passing `set_swap_interval` call) is proven -- see [3] below.
//     A THIRD gap, found by adversarial review (w18-rev2) rather than declared up front here:
//     `WindowGlfw::monitor_for_window()`'s largest-overlap ARITHMETIC (window_glfw.hpp) has no
//     automated proof it picks the CORRECT monitor among several candidates -- Xvfb always
//     reports exactly one virtual monitor, so this suite cannot distinguish that arithmetic from
//     "always return the primary monitor" (measured: sabotaging it to do exactly that survives
//     the full 102/102 suite). See that method's own doc-comment for the full writeup.
//
//     FORWARD-COVERAGE gate (same shape as `app_font_face_sanity.cpp`'s rationale):
//     `App::set_swap_interval`/`get_monitor_refresh_hz` are thin forwards to
//     `WindowGlfw::set_swap_interval`/`get_monitor_refresh_hz` (src/app.cpp) -- a broken forward
//     on the App side (e.g. `App::set_swap_interval` short-circuited to always `true`, never
//     reaching WindowGlfw's own negative-interval guard) would be invisible to a
//     WindowGlfw-only test. [1]/[2] below exercise WindowGlfw DIRECTLY (same
//     `#include "../src/window_glfw.hpp"` pattern as window_smoke.cpp) to pin the real guard
//     behaviour; [3] exercises the SAME guard through `App` specifically, so a broken forward
//     shows up as a [3] failure even though [1]/[2] would still pass.
//
//     [1] PRE-CREATE GUARD (WindowGlfw, `win_ == nullptr`) -- `set_swap_interval` returns
//         `false`, `get_monitor_refresh_hz` returns `<= 0`. No crash (D5 fail-high, twin of
//         every other WindowGlfw guard in this class -- mode()/set_mode() etc.).
//     [2] POST-CREATE CONTRACT (WindowGlfw, real window) -- negative interval rejected
//         (`false`); `0`/`1`/`3` accepted (`true`) including idempotent re-requests of the SAME
//         value; `get_monitor_refresh_hz()` is deterministic (two calls, same answer) and
//         `>= 0` (never negative -- GLFWvidmode::refreshRate is documented non-negative).
//     [3] APP FORWARD-COVERAGE -- the SAME negative-interval guard, reached through
//         `App::set_swap_interval`; `App::set_vsync(true/false)` both succeed;
//         `App::get_monitor_refresh_hz()` is deterministic through the App facade too. Ends
//         with one render() to confirm the pipeline stays live after the setter calls (same
//         "does not break anything else" proof app_dp_ratio_smoke.cpp uses).
//
// PT: app_vsync_sanity -- teste de contrato para `WM-VSYNC` (onda de fechamento de
//     window-modes, consumer-driven, escopo aprovado pelo líder 2026-07-23):
//     `App::set_swap_interval(int)`/`App::set_vsync(bool)` (açúcar sobre o primeiro) e
//     `App::get_monitor_refresh_hz()`.
//
//     HONESTIDADE HEADLESS (declarada, não forjada -- ver a própria política de review da casa
//     de fazer downgrade em vez de inventar cobertura): sob Xvfb/Mesa-llvmpipe não há display
//     real, nenhum ritmo real de compositor, e nenhuma query do GLFW que ecoe de volta o
//     intervalo de swap que um driver de fato aplicou -- `glfwSwapInterval` é fire-and-forget
//     na própria superfície de API do GLFW. Este arquivo portanto prova o CONTRATO (guardas,
//     idempotência, forward), nunca o EFEITO REAL de vsync (nenhuma asserção de tempo de frame
//     em lugar nenhum deste arquivo -- isso seria flaky por construção sob um driver sem
//     refresh real pra ritmar contra). Duas propriedades específicas são INVERIFICÁVEIS aqui e
//     são nomeadas em vez de silenciosamente puladas:
//       - Se o driver/compositor da plataforma de fato HONRA um intervalo pedido (a própria
//         nota "VSYNC HONESTY" do D9 em app.hpp documenta a mesma classe de limitação pro ritmo
//         pré-existente de render()/run()).
//       - O mapeamento específico true->1 / false->0 do `App::set_vsync(bool)` não dá pra
//         distinguir pelo efeito OBSERVÁVEL (nenhuma query existe); só que AMBAS as chamadas
//         têm sucesso (alcançam uma chamada `set_swap_interval` que passa pela guarda) é provado
//         -- ver [3] abaixo.
//     Uma TERCEIRA lacuna, achada por review adversarial (w18-rev2) em vez de declarada de
//     antemão aqui: a ARITMÉTICA de maior overlap de `WindowGlfw::monitor_for_window()`
//     (window_glfw.hpp) não tem prova automatizada de que escolhe o monitor CORRETO entre vários
//     candidatos -- o Xvfb sempre reporta exatamente um monitor virtual, então esta suíte não
//     distingue aquela aritmética de "sempre retorna o monitor primário" (medido: sabotar pra
//     fazer exatamente isso sobrevive à suíte completa 102/102). Ver o próprio doc-comment
//     daquele método pro relato completo.
//
//     Gate de COBERTURA DE FORWARD (mesmo formato do racional de `app_font_face_sanity.cpp`):
//     `App::set_swap_interval`/`get_monitor_refresh_hz` são repasses finos a
//     `WindowGlfw::set_swap_interval`/`get_monitor_refresh_hz` (src/app.cpp) -- um forward
//     quebrado do lado App (ex.: `App::set_swap_interval` curto-circuitado pra sempre `true`,
//     nunca alcançando a própria guarda de intervalo-negativo do WindowGlfw) seria invisível a
//     um teste só-WindowGlfw. [1]/[2] abaixo exercitam o WindowGlfw DIRETAMENTE (mesmo padrão
//     `#include "../src/window_glfw.hpp"` de window_smoke.cpp) pra fixar o comportamento real da
//     guarda; [3] exercita a MESMA guarda através do `App` especificamente, então um forward
//     quebrado aparece como falha em [3] mesmo que [1]/[2] ainda passem.
//
//     [1] GUARDA PRÉ-CREATE (WindowGlfw, `win_ == nullptr`) -- `set_swap_interval` retorna
//         `false`, `get_monitor_refresh_hz` retorna `<= 0`. Sem crash (fail-high D5, gêmeo de
//         toda outra guarda desta classe -- mode()/set_mode() etc.).
//     [2] CONTRATO PÓS-CREATE (WindowGlfw, janela real) -- intervalo negativo rejeitado
//         (`false`); `0`/`1`/`3` aceitos (`true`) inclusive repedidos idempotentes do MESMO
//         valor; `get_monitor_refresh_hz()` é determinístico (duas chamadas, mesma resposta) e
//         `>= 0` (nunca negativo -- GLFWvidmode::refreshRate é documentado não-negativo).
//     [3] COBERTURA DE FORWARD DO APP -- a MESMA guarda de intervalo-negativo, alcançada através
//         de `App::set_swap_interval`; `App::set_vsync(true/false)` ambos têm sucesso;
//         `App::get_monitor_refresh_hz()` é determinístico também pela fachada App. Termina com
//         um render() pra confirmar que o pipeline continua ativo após as chamadas de setter
//         (mesma prova de "não quebra mais nada" que app_dp_ratio_smoke.cpp usa).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  int failures = 0;
  // EN: Captured by [2], cross-checked by [3] against App::get_monitor_refresh_hz() -- closes a
  //     coverage gap the determinism check (hz_a == hz_b) alone does NOT: under this Xvfb
  //     environment the real answer happens to be a stable `0`, so a HARDCODED-constant mutation
  //     of App::get_monitor_refresh_hz() (e.g. always returning `60`, never forwarding to
  //     WindowGlfw) would still be "deterministic" and pass that check alone. Comparing against
  //     the value [2] already proved comes from a REAL WindowGlfw::get_monitor_refresh_hz() call
  //     catches exactly that mutation.
  // PT: Capturado por [2], contra-checado por [3] contra App::get_monitor_refresh_hz() -- fecha
  //     uma lacuna de cobertura que o cheque de determinismo (hz_a == hz_b) sozinho NÃO fecha:
  //     sob este ambiente Xvfb a resposta real por acaso é um `0` estável, então uma mutação de
  //     App::get_monitor_refresh_hz() pra uma constante HARDCODED (ex.: sempre retornar `60`,
  //     nunca encaminhando ao WindowGlfw) ainda seria "determinística" e passaria aquele cheque
  //     sozinho. Comparar contra o valor que [2] já provou vir de uma chamada REAL a
  //     WindowGlfw::get_monitor_refresh_hz() pega exatamente essa mutação.
  int hz_from_window_glfw = -1;

  // ---------------------------------------------------------------------------
  // [1] Pre-create guard -- a default-constructed WindowGlfw never had create() called.
  // ---------------------------------------------------------------------------
  {
    glintfx::WindowGlfw w;
    if (w.set_swap_interval(1)) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [1] set_swap_interval(1) returned true "
                   "before create()\n");
      ++failures;
    }
    if (w.get_monitor_refresh_hz() > 0) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [1] get_monitor_refresh_hz() > 0 "
                   "before create()\n");
      ++failures;
    }
  }

  // ---------------------------------------------------------------------------
  // [2] Post-create contract, exercised directly on WindowGlfw (scoped so its dtor -- and the
  //     glfwTerminate() it calls -- runs BEFORE the App in [3] below calls glfwInit() again;
  //     two live GLFW window objects in the same process at once is an untested, unproven
  //     pattern in this suite -- see window_glfw.hpp's own N3-adjacent single-window posture).
  // ---------------------------------------------------------------------------
  {
    glintfx::WindowGlfw w;
    if (!w.create("app_vsync_sanity", 320, 240)) {
      std::puts("SKIP: WindowGlfw::create() failed -- no display / GL context");
      return 0;
    }

    if (w.set_swap_interval(-1)) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [2] set_swap_interval(-1) returned true "
                   "(negative/adaptive interval must be rejected, out of scope)\n");
      ++failures;
    }
    if (!w.set_swap_interval(0)) {
      std::fprintf(stderr, "app_vsync_sanity FAIL: [2] set_swap_interval(0) returned false\n");
      ++failures;
    }
    if (!w.set_swap_interval(1)) {
      std::fprintf(stderr, "app_vsync_sanity FAIL: [2] set_swap_interval(1) returned false\n");
      ++failures;
    }
    if (!w.set_swap_interval(1)) { // EN: idempotent re-request. PT: repedido idempotente.
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [2] set_swap_interval(1) (2nd, idempotent) "
                   "returned false\n");
      ++failures;
    }
    if (!w.set_swap_interval(3)) { // EN: n>1, half/third-rate territory. PT: n>1, meia/terça-taxa.
      std::fprintf(stderr, "app_vsync_sanity FAIL: [2] set_swap_interval(3) returned false\n");
      ++failures;
    }

    const int hz_a = w.get_monitor_refresh_hz();
    const int hz_b = w.get_monitor_refresh_hz();
    if (hz_a != hz_b) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [2] get_monitor_refresh_hz() not deterministic "
                   "(%d then %d)\n",
                   hz_a, hz_b);
      ++failures;
    }
    if (hz_a < 0) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [2] get_monitor_refresh_hz() returned negative "
                   "(%d) -- GLFWvidmode::refreshRate is documented non-negative\n",
                   hz_a);
      ++failures;
    }
    std::printf(
        "app_vsync_sanity [2] WindowGlfw::get_monitor_refresh_hz() = %d Hz "
        "(0 is a documented, expected headless/indeterminate result, not a failure)\n",
        hz_a);
    hz_from_window_glfw = hz_a;
  }

  // ---------------------------------------------------------------------------
  // [3] App forward-coverage -- same guard/query surface, reached through the App facade.
  // ---------------------------------------------------------------------------
  {
    glintfx::AppConfig cfg;
    cfg.title = "app_vsync_sanity_app";
    cfg.width = 320;
    cfg.height = 240;
    glintfx::App app(cfg);
    if (!app.ok()) {
      std::puts("SKIP: App init failed -- no display / GL context");
      return failures; // EN: report any [1]/[2] failures already found. PT: reporta falhas de [1]/[2] já achadas.
    }

    if (app.set_swap_interval(-1)) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [3] App::set_swap_interval(-1) returned true -- "
                   "forward to WindowGlfw's negative-interval guard broken?\n");
      ++failures;
    }
    if (!app.set_swap_interval(0)) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [3] App::set_swap_interval(0) returned false\n");
      ++failures;
    }
    if (!app.set_vsync(true)) {
      std::fprintf(stderr, "app_vsync_sanity FAIL: [3] App::set_vsync(true) returned false\n");
      ++failures;
    }
    if (!app.set_vsync(false)) {
      std::fprintf(stderr, "app_vsync_sanity FAIL: [3] App::set_vsync(false) returned false\n");
      ++failures;
    }

    const int hz_a = app.get_monitor_refresh_hz();
    const int hz_b = app.get_monitor_refresh_hz();
    if (hz_a != hz_b) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [3] App::get_monitor_refresh_hz() not deterministic "
                   "(%d then %d)\n",
                   hz_a, hz_b);
      ++failures;
    }
    if (hz_a < 0) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [3] App::get_monitor_refresh_hz() returned negative "
                   "(%d)\n",
                   hz_a);
      ++failures;
    }
    std::printf(
        "app_vsync_sanity [3] App::get_monitor_refresh_hz() = %d Hz "
        "(0 is a documented, expected headless/indeterminate result, not a failure)\n",
        hz_a);

    // EN: Forward-coverage cross-check (see the header-comment note on hz_from_window_glfw at
    //     the top of main()) -- App::get_monitor_refresh_hz() must agree with the DIRECT
    //     WindowGlfw call [2] already proved was live, not a hardcoded stand-in.
    // PT: Contra-cheque de cobertura de forward (ver a nota do comentário de cabeçalho sobre
    //     hz_from_window_glfw no topo de main()) -- App::get_monitor_refresh_hz() precisa
    //     concordar com a chamada DIRETA ao WindowGlfw que [2] já provou ser ao vivo, não um
    //     substituto hardcoded.
    if (hz_a != hz_from_window_glfw) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [3] App::get_monitor_refresh_hz() = %d disagrees "
                   "with WindowGlfw::get_monitor_refresh_hz() = %d from [2] -- forward broken "
                   "or a hardcoded stand-in?\n",
                   hz_a, hz_from_window_glfw);
      ++failures;
    }

    // EN: Confirm the pipeline stays live after the setter calls (same proof pattern as
    //     app_dp_ratio_smoke.cpp's step 4).
    // PT: Confirma que o pipeline continua ativo após as chamadas de setter (mesmo padrão de
    //     prova do passo 4 de app_dp_ratio_smoke.cpp).
    app.load("tests/min.rml");
    app.poll_events();
    app.update();
    app.render();
    if (!app.ok()) {
      std::fprintf(stderr,
                   "app_vsync_sanity FAIL: [3] App::ok() false after render() following the "
                   "vsync setter calls\n");
      ++failures;
    }
  }

  if (failures == 0) {
    std::puts("app_vsync_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "app_vsync_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
