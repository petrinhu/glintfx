// SPDX-License-Identifier: MPL-2.0
// EN: App resize regression smoke (AUD-PUB-1, v0.5.0). Proves App::render() re-flows the
//     RmlUi layout when the GLFW window's framebuffer size changes between frames. Also
//     proves the AUD-PUB-1-TWIN fix (Step 3 below): App::snapshot() re-flows the layout too,
//     even when called WITHOUT an intervening render().
//
//     BUG (pre-fix): App::render() (app.cpp) read the current window size and passed it to
//     Engine::render_standalone(w, h), but never called Engine::set_viewport(w, h) --
//     the only path that reaches Rml::Context::SetDimensions() (engine.cpp). The window
//     resized at the GLFW/GL level (viewport, framebuffer) but the RmlUi layout stayed
//     frozen at the AppConfig construction-time dimensions forever. UiLayer never had this
//     bug because embed hosts call set_viewport() explicitly every frame/resize; App has
//     no equivalent public API and must do it internally (Option A, AUD-PUB-1: automatic,
//     zero new public surface).
//
//     TWIN BUG (found by adversarial review, v0.5.0): App::snapshot() had the identical bug,
//     independently of render() -- it called render_standalone(w, h) directly with no
//     set_viewport() at all, and did not share render()'s last_render_w/h cache. A host that
//     resizes and then calls ONLY snapshot() (no render() in between) captured a PPM with a
//     layout frozen at the previous size. Fixed by extracting render()'s diff-and-set_viewport
//     logic into App::Impl::sync_viewport(w, h), now called by both render() and snapshot().
//
//     ORACLE: resize_scene.rml/.rcss defines #probe as display:block, width:100%,
//     height:100%, zero margin/border/padding -- its border-box is EXACTLY the viewport
//     (context) size. get_element_box("probe").w/h is therefore a deterministic proxy for
//     "did Context::SetDimensions get called with the new size".
//
//     TEST-ONLY GLFW DEPENDENCY: App does not expose its GLFWwindow* (by design -- no
//     third-party type crosses the public glintfx:: boundary). This test forces a resize
//     by calling glfwSetWindowSize() directly on the window returned by
//     glfwGetCurrentContext(): WindowGlfw::create() (app.cpp -> window_glfw.cpp) calls
//     glfwMakeContextCurrent() during App construction and nothing else changes the
//     current context afterwards, and glintfx enforces a single App instance per process
//     (N3, app.hpp), so glfwGetCurrentContext() deterministically returns the App's own
//     window handle here. Same test-only-GLFW-dependency pattern already used by the
//     embed-mode tests (see tests/CMakeLists.txt "GL context fixture" comment); this file
//     links the `glfw` target directly for the header/symbols (glintfx links glfw
//     PRIVATE, so it is not propagated to consumers, same rationale as golden_test linking
//     gl3w directly).
//
// PT: Smoke de regressão de resize do App (AUD-PUB-1, v0.5.0). Prova que App::render()
//     refaz o layout do RmlUi quando o tamanho do framebuffer da janela GLFW muda entre
//     frames. Também prova o fix AUD-PUB-1-TWIN (Passo 3 abaixo): App::snapshot() também
//     refaz o layout, mesmo quando chamado SEM um render() no meio.
//
//     BUG (pré-fix): App::render() (app.cpp) lia o tamanho atual da janela e passava para
//     Engine::render_standalone(w, h), mas nunca chamava Engine::set_viewport(w, h) --
//     o único caminho que alcança Rml::Context::SetDimensions() (engine.cpp). A janela
//     redimensionava no nível GLFW/GL (viewport, framebuffer) mas o layout do RmlUi ficava
//     congelado para sempre nas dimensões de construção do AppConfig. O UiLayer nunca teve
//     esse bug porque hosts embed chamam set_viewport() explicitamente a cada frame/resize;
//     o App não tem API pública equivalente e precisa fazer isso internamente (Opção A,
//     AUD-PUB-1: automático, zero superfície pública nova).
//
//     BUG GÊMEO (achado por review adversarial, v0.5.0): App::snapshot() tinha o bug idêntico,
//     independente do render() -- chamava render_standalone(w, h) direto sem nenhum
//     set_viewport(), e não compartilhava o cache last_render_w/h do render(). Um host que
//     redimensiona e depois chama SÓ snapshot() (sem render() no meio) capturava um PPM com
//     layout congelado no tamanho anterior. Corrigido extraindo a lógica de
//     diff-e-set_viewport do render() para App::Impl::sync_viewport(w, h), agora chamada por
//     ambos render() e snapshot().
//
//     ORÁCULO: resize_scene.rml/.rcss define #probe como display:block, width:100%,
//     height:100%, sem margem/borda/padding -- seu border-box é EXATAMENTE o tamanho da
//     viewport (contexto). get_element_box("probe").w/h é portanto um proxy determinístico
//     para "Context::SetDimensions foi chamado com o novo tamanho".
//
//     DEPENDÊNCIA DE GLFW SÓ-DE-TESTE: o App não expõe seu GLFWwindow* (por desenho --
//     nenhum tipo de terceiro cruza a fronteira pública glintfx::). Este teste força um
//     resize chamando glfwSetWindowSize() diretamente na janela retornada por
//     glfwGetCurrentContext(): WindowGlfw::create() (app.cpp -> window_glfw.cpp) chama
//     glfwMakeContextCurrent() durante a construção do App e nada mais troca o contexto
//     corrente depois disso, e o glintfx impõe uma única instância de App por processo
//     (N3, app.hpp), então glfwGetCurrentContext() retorna deterministicamente o handle da
//     janela do próprio App aqui. Mesmo padrão de dependência-de-GLFW-só-de-teste já usado
//     pelos testes de embed mode (ver comentário "GL context fixture" em
//     tests/CMakeLists.txt); este arquivo linka o target `glfw` diretamente pelo
//     header/símbolos (glintfx linka glfw PRIVATE, então não é propagado a consumidores,
//     mesma racional de golden_test linkando gl3w diretamente).
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>

namespace {
// EN: Tolerance in pixels -- loose enough to absorb any dp_ratio=1.0 rounding in RmlUi's
//     layout engine, tight enough that a frozen (unchanged) layout is unambiguously caught.
// PT: Tolerância em pixels -- larga o bastante para absorver arredondamento do motor de
//     layout do RmlUi com dp_ratio=1.0, estreita o bastante para capturar sem ambiguidade
//     um layout congelado (inalterado).
constexpr float kTolPx = 2.0f;
bool close_to(float actual, float expected) {
  return std::fabs(actual - expected) <= kTolPx;
}
} // namespace

int main() {
  glintfx::App app({ .title = "app_resize_smoke", .width = 400, .height = 300 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention -- see
  //     app_element_box_smoke.cpp); resize_scene.rml is copied to CMAKE_CURRENT_BINARY_DIR
  //     (build/tests/) by tests/CMakeLists.txt, so it is addressed with the "tests/" prefix.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo -- ver
  //     app_element_box_smoke.cpp); resize_scene.rml é copiado para CMAKE_CURRENT_BINARY_DIR
  //     (build/tests/) por tests/CMakeLists.txt, então é endereçado com o prefixo "tests/".
  if (!app.load("tests/resize_scene.rml")) { std::puts("FAIL: load"); return 2; }

  // ---------------------------------------------------------------------------
  // EN: Step 1 -- one frame at the construction size (400x300). #probe's border-box must
  //     match it exactly (width:100%/height:100%, zero margin/border/padding oracle).
  // PT: Passo 1 -- um frame no tamanho de construção (400x300). O border-box do #probe deve
  //     bater exatamente (oráculo width:100%/height:100%, sem margem/borda/padding).
  // ---------------------------------------------------------------------------
  app.update();
  app.render();
  auto before = app.get_element_box("probe");
  if (!before.found) { std::puts("FAIL: probe not found (before resize)"); return 3; }
  std::printf("app_resize_smoke: before resize box = %.1fx%.1f\n", before.w, before.h);
  if (!close_to(before.w, 400.0f) || !close_to(before.h, 300.0f)) {
    std::printf("FAIL: initial box %.1fx%.1f does not match construction size 400x300\n",
                before.w, before.h);
    return 4;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 -- force the OS-level window resize. App does not expose its GLFWwindow*
  //     (no third-party type crosses the public boundary), so this test-only fixture grabs
  //     it via glfwGetCurrentContext() -- see file header for why that is deterministic here.
  // PT: Passo 2 -- força o resize da janela no nível do SO. O App não expõe seu
  //     GLFWwindow* (nenhum tipo de terceiro cruza a fronteira pública), então este fixture
  //     só-de-teste o obtém via glfwGetCurrentContext() -- ver cabeçalho do arquivo pra
  //     entender por que isso é determinístico aqui.
  // ---------------------------------------------------------------------------
  GLFWwindow* win = glfwGetCurrentContext();
  if (!win) { std::puts("FAIL: glfwGetCurrentContext() returned null"); return 5; }
  glfwSetWindowSize(win, 800, 600);

  // EN: Two frames: poll_events() drains the ConfigureNotify/framebuffer-size events that
  //     make WindowGlfw::size() (glfwGetFramebufferSize) observe the new size, then render()
  //     must pick it up and re-flow the layout (the fix under test).
  // PT: Dois frames: poll_events() drena os eventos ConfigureNotify/framebuffer-size que
  //     fazem WindowGlfw::size() (glfwGetFramebufferSize) observar o novo tamanho, depois
  //     render() precisa capturar isso e refazer o layout (o fix sob teste).
  app.poll_events();
  app.update();
  app.render();
  app.poll_events();
  app.update();
  app.render();

  auto after = app.get_element_box("probe");
  if (!after.found) { std::puts("FAIL: probe not found (after resize)"); return 6; }
  std::printf("app_resize_smoke: after resize box = %.1fx%.1f\n", after.w, after.h);
  if (!close_to(after.w, 800.0f) || !close_to(after.h, 600.0f)) {
    std::printf("FAIL: box after resize %.1fx%.1f did not follow window resize to 800x600 "
                "(App::render() is not calling Engine::set_viewport() -- AUD-PUB-1 "
                "regression)\n", after.w, after.h);
    return 7;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 (AUD-PUB-1-TWIN, v0.5.0) -- resize AGAIN, but this time drive the two settle
  //     frames with snapshot() instead of render(), and NEVER call render() again for the rest
  //     of this test. Before the fix, snapshot() never called set_viewport(), so the layout
  //     would stay frozen at the Step-2 size (800x600) forever, even though the window itself
  //     resized again to 500x350 at the GLFW/GL level. get_element_box("probe") is the same
  //     deterministic oracle as Steps 1/2 -- it reads back whatever size
  //     Rml::Context::SetDimensions() was last called with, regardless of which public method
  //     (render() or snapshot()) triggered it.
  //
  //     TWO-FRAME SETTLE, SAME AS STEP 2 (not a new requirement invented for this step): under
  //     Xvfb/GLFW, a single poll_events() after glfwSetWindowSize() is not enough for
  //     get_element_box() to observe the new size -- Rml::Context::SetDimensions() only marks
  //     the layout DIRTY (Context.cpp), the actual re-format happens inside the NEXT
  //     Rml::Context::Update() call, which Engine::update() pumps. So frame N's
  //     render()/snapshot() call is what OBSERVES the resized window and calls SetDimensions();
  //     frame N+1's update() is what ACTUALLY re-flows the layout for get_element_box() to read
  //     back. Step 2 already relies on exactly this two-frame shape (see its own comment above)
  //     -- Step 3 reproduces it faithfully, substituting render() with snapshot() at the one
  //     call that must prove the fix (frame 1), while frame 2 stays a plain update() with NO
  //     render() call anywhere in this step -- render() is the ONLY thing that used to touch
  //     set_viewport() at all, so proving the layout re-flows correctly with it NEVER called
  //     again from this point on is exactly what AUD-PUB-1-TWIN needs.
  // PT: Passo 3 (AUD-PUB-1-TWIN, v0.5.0) -- redimensiona DE NOVO, mas desta vez conduz os dois
  //     frames de assentamento com snapshot() em vez de render(), e NUNCA mais chama render()
  //     pelo resto deste teste. Antes do fix, snapshot() nunca chamava set_viewport(), então o
  //     layout ficaria congelado no tamanho do Passo 2 (800x600) para sempre, mesmo com a
  //     janela em si tendo redimensionado de novo para 500x350 no nível GLFW/GL.
  //     get_element_box("probe") é o mesmo oráculo determinístico dos Passos 1/2 -- lê de volta
  //     qualquer tamanho que Rml::Context::SetDimensions() tenha recebido por último,
  //     independente de qual método público (render() ou snapshot()) disparou a chamada.
  //
  //     ASSENTAMENTO EM DOIS FRAMES, IGUAL AO PASSO 2 (não é um requisito novo inventado para
  //     este passo): sob Xvfb/GLFW, um único poll_events() após glfwSetWindowSize() não basta
  //     para get_element_box() observar o novo tamanho -- Rml::Context::SetDimensions() só
  //     marca o layout SUJO (Context.cpp), a reformatação de fato acontece dentro da PRÓXIMA
  //     chamada a Rml::Context::Update(), que Engine::update() bombeia. Então o
  //     render()/snapshot() do frame N é quem OBSERVA a janela redimensionada e chama
  //     SetDimensions(); o update() do frame N+1 é quem DE FATO refaz o layout para
  //     get_element_box() ler de volta. O Passo 2 já depende exatamente dessa forma de dois
  //     frames (ver o comentário dele acima) -- o Passo 3 a reproduz fielmente, substituindo
  //     render() por snapshot() na única chamada que precisa provar o fix (frame 1), enquanto o
  //     frame 2 permanece um update() simples SEM nenhuma chamada a render() em lugar nenhum
  //     deste passo -- render() era a ÚNICA coisa que tocava set_viewport() antes, então provar
  //     que o layout refaz corretamente com ele NUNCA MAIS chamado a partir daqui é exatamente
  //     o que o AUD-PUB-1-TWIN precisa.
  // ---------------------------------------------------------------------------
  glfwSetWindowSize(win, 500, 350);

  const char* kSnapshotPath = "app_resize_smoke_snapshot.ppm";
  app.poll_events();
  app.update();
  if (!app.snapshot(kSnapshotPath)) { std::puts("FAIL: snapshot() (frame 1) returned false"); return 8; }
  app.poll_events();
  app.update();
  if (!app.snapshot(kSnapshotPath)) { std::puts("FAIL: snapshot() (frame 2) returned false"); return 9; }

  auto after_snapshot = app.get_element_box("probe");
  if (!after_snapshot.found) { std::puts("FAIL: probe not found (after snapshot-only resize)"); return 10; }
  std::printf("app_resize_smoke: after snapshot-only resize box = %.1fx%.1f\n",
              after_snapshot.w, after_snapshot.h);
  if (!close_to(after_snapshot.w, 500.0f) || !close_to(after_snapshot.h, 350.0f)) {
    std::printf("FAIL: box after snapshot-only resize %.1fx%.1f did not follow window resize to "
                "500x350 (App::snapshot() is not calling sync_viewport() -- AUD-PUB-1-TWIN "
                "regression)\n", after_snapshot.w, after_snapshot.h);
    return 11;
  }

  std::puts("app_resize_smoke OK");
  return 0;
}
