// SPDX-License-Identifier: MPL-2.0
// EN: A1 (framework-2D) -- verifies App::set_frame_callback(): the game's per-frame draw hook.
//     (1) Smoke: N frames of poll/update/render -- the counter reaches exactly N, every
//     dt_seconds is >= 0 and sane (< 10s -- generous ceiling, this whole test runs in well
//     under a second under Xvfb), and the FIRST call reports dt_seconds == 0 (no meaningful
//     previous frame to measure against -- see App::set_frame_callback's doc-comment,
//     app.hpp).
//     (2) Pixel-proof leg (the plan's "perna forte"): inside the hook, paint the EXACT
//     screen-space region app_process_event_scene.rcss's opaque #btn_a occupies (10,10..90,50)
//     with a scissor+clear to a colour (bright green) that appears NOWHERE in the loaded
//     document (body #101010, btn_a #ff0000). App::snapshot() -- routed through the SAME
//     App::Impl::render_frame() helper as render() (see app.cpp) so the hook fires here too --
//     then captures the composited frame. The PPM's pixel at btn_a's centre is asserted to be
//     RED (btn_a's own colour), never the hook's green: since the hook runs BEFORE
//     Context::Render() (Engine::render_standalone's documented insertion point, engine.hpp),
//     RmlUi's own opaque #btn_a quad draws OVER whatever the hook painted there, proving BOTH
//     that the scene composes UNDER the UI (the ordering contract) AND that the GL context was
//     genuinely current inside the hook (the clear call had a real, visible effect that
//     something then had to paint over -- a no-op/broken context would leave body's #101010
//     background instead, not a colour transition FROM the hook's green TO btn_a's red).
// PT: A1 (framework-2D) -- verifica App::set_frame_callback(): o hook de desenho por-frame do
//     jogo.
//     (1) Fumaça: N frames de poll/update/render -- o contador chega a exatamente N, todo
//     dt_seconds é >= 0 e são (< 10s -- teto generoso, este teste inteiro roda bem abaixo de
//     um segundo sob Xvfb), e a PRIMEIRA chamada reporta dt_seconds == 0 (nenhum frame anterior
//     significativo pra medir contra -- ver o doc-comment de App::set_frame_callback, app.hpp).
//     (2) Perna de prova por pixel (a "perna forte" do plano): dentro do hook, pinta a região
//     EXATA em espaço de tela que o #btn_a opaco de app_process_event_scene.rcss ocupa
//     (10,10..90,50) com um scissor+clear pra uma cor (verde vivo) que não aparece EM LUGAR
//     NENHUM do documento carregado (body #101010, btn_a #ff0000). App::snapshot() --
//     roteado pelo MESMO helper App::Impl::render_frame() que o render() (ver app.cpp), então
//     o hook dispara aqui também -- então captura o frame composto. O pixel do PPM no centro do
//     btn_a é asserido como VERMELHO (a própria cor do btn_a), nunca o verde do hook: já que o
//     hook roda ANTES de Context::Render() (o ponto de inserção documentado de
//     Engine::render_standalone, engine.hpp), o próprio quad opaco #btn_a do RmlUi desenha POR
//     CIMA do que o hook pintou lá, provando TANTO que a cena compõe SOB a UI (o contrato de
//     ordem) QUANTO que o contexto GL estava de fato corrente dentro do hook (a chamada de
//     clear teve efeito real e visível, que algo então teve que pintar por cima -- um contexto
//     no-op/quebrado deixaria o background #101010 do body no lugar, não uma transição de cor
//     DO verde do hook PARA o vermelho do btn_a).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"
#include <cstdio>
#include <cstdlib>

int main() {
  glintfx::App app({ .title = "app_frame_callback_smoke", .width = 300, .height = 200 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); reuses
  //     app_process_event_scene.rml/.rcss (copied for app_process_event_smoke, same
  //     tests/CMakeLists.txt block) -- #btn_a's known geometry/colour is this test's oracle.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo); reusa
  //     app_process_event_scene.rml/.rcss (copiado pro app_process_event_smoke, mesmo bloco de
  //     tests/CMakeLists.txt) -- a geometria/cor conhecida do #btn_a é o oráculo deste teste.
  if (!app.load("tests/app_process_event_scene.rml")) { std::puts("FAIL: load"); return 2; }

  // ---------------------------------------------------------------------------
  // EN: (1) Smoke: counter + dt sanity over N frames.
  // PT: (1) Fumaça: contador + sanidade de dt ao longo de N frames.
  // ---------------------------------------------------------------------------
  int   call_count   = 0;
  float first_dt      = -1.0f;
  bool  saw_negative_dt = false;
  bool  saw_insane_dt   = false;

  app.set_frame_callback([&](float dt) {
    if (call_count == 0) first_dt = dt;
    if (dt < 0.0f)  saw_negative_dt = true;
    if (dt >= 10.0f) saw_insane_dt = true;
    ++call_count;
  });

  constexpr int kFrames = 5;
  for (int i = 0; i < kFrames; ++i) {
    app.poll_events();
    app.update();
    app.render();
  }

  if (call_count != kFrames) {
    std::fprintf(stderr, "FAIL: expected %d hook calls, got %d\n", kFrames, call_count);
    return 3;
  }
  if (first_dt != 0.0f) {
    std::fprintf(stderr, "FAIL: first dt_seconds=%.6f, expected exactly 0.0\n", first_dt);
    return 4;
  }
  if (saw_negative_dt) { std::puts("FAIL: a negative dt_seconds was observed"); return 5; }
  if (saw_insane_dt)   { std::puts("FAIL: a dt_seconds >= 10s was observed (sanity ceiling)"); return 6; }
  if (!app.ok())        { std::puts("FAIL: ok() false after frame loop"); return 7; }

  // ---------------------------------------------------------------------------
  // EN: (2) Pixel-proof leg -- see file header for the full oracle derivation.
  //     #btn_a occupies (10,10)..(90,50) in app_process_event_scene.rcss; (50, 30) is its
  //     centre, safely inside on every side.
  // PT: (2) Perna de prova por pixel -- ver cabeçalho do arquivo pra derivação completa do
  //     oráculo. #btn_a ocupa (10,10)..(90,50) em app_process_event_scene.rcss; (50, 30) é o
  //     centro dele, seguramente dentro por todos os lados.
  // ---------------------------------------------------------------------------
  app.set_frame_callback([](float) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(10, 200 - 50, 80, 40);  // EN: GL is bottom-left origin; window is 300x200.
                                       // PT: GL tem origem inferior-esquerda; janela é 300x200.
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);  // EN: bright green -- appears nowhere in the doc.
                                            // PT: verde vivo -- não aparece em lugar nenhum do doc.
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
  });

  const char* kSnapshotPath = "app_frame_callback_smoke.ppm";
  if (!app.snapshot(kSnapshotPath)) { std::puts("FAIL: snapshot() returned false"); return 8; }

  FILE* f = std::fopen(kSnapshotPath, "rb");
  if (!f) { std::puts("FAIL: could not reopen snapshot PPM"); return 9; }
  char magic[3] = {0};
  int w = 0, h = 0, maxval = 0;
  if (std::fscanf(f, "%2s %d %d %d", magic, &w, &h, &maxval) != 4 ||
      magic[0] != 'P' || magic[1] != '6') {
    std::puts("FAIL: malformed PPM header");
    std::fclose(f);
    return 10;
  }
  std::fgetc(f);  // EN: single whitespace byte after the header, per the P6 format.
                   // PT: um byte de espaço em branco após o cabeçalho, conforme o formato P6.
  if (w != 300 || h != 200) {
    std::fprintf(stderr, "FAIL: snapshot size %dx%d, expected 300x200\n", w, h);
    std::fclose(f);
    return 11;
  }
  // EN: PPM row order is top-left origin (App::snapshot() already flips glReadPixels'
  //     bottom-left rows when writing the file -- see app.cpp's snapshot() doc-comment), so
  //     window-space (50, 30) maps directly to row 30, column 50 here.
  // PT: A ordem de linhas do PPM é origem superior-esquerda (App::snapshot() já inverte as
  //     linhas bottom-left do glReadPixels ao gravar o arquivo -- ver o doc-comment de
  //     snapshot() em app.cpp), então o espaço-janela (50, 30) mapeia direto pra linha 30,
  //     coluna 50 aqui.
  const long offset = static_cast<long>(30) * w * 3 + static_cast<long>(50) * 3;
  if (std::fseek(f, offset, SEEK_CUR) != 0) {
    std::puts("FAIL: fseek into pixel data failed");
    std::fclose(f);
    return 12;
  }
  unsigned char rgb[3] = {0, 0, 0};
  if (std::fread(rgb, 1, 3, f) != 3) {
    std::puts("FAIL: short read of pixel data");
    std::fclose(f);
    return 13;
  }
  std::fclose(f);

  std::printf("app_frame_callback_smoke: btn_a centre pixel = (%d, %d, %d)\n", rgb[0], rgb[1], rgb[2]);
  // EN: Loose thresholds tolerant of llvmpipe AA/blend at the rect edge (this sample point is
  //     deep in the interior, so the margin is generous defence-in-depth, not a requirement).
  // PT: Limiares frouxos, tolerantes a AA/blend do llvmpipe na borda do retângulo (este ponto
  //     de amostra está fundo no interior, então a margem é defesa-em-profundidade generosa,
  //     não um requisito).
  const bool looks_red   = rgb[0] > 180 && rgb[1] < 60  && rgb[2] < 60;
  const bool looks_green = rgb[1] > 180 && rgb[0] < 60  && rgb[2] < 60;
  if (looks_green) {
    std::puts("FAIL: pixel is the hook's green -- scene composed OVER the UI, not under it "
              "(or the UI render pass never ran / never covered it)");
    return 14;
  }
  if (!looks_red) {
    std::fprintf(stderr,
                 "FAIL: pixel is neither btn_a's red nor the hook's green (%d,%d,%d) -- "
                 "hook likely never ran / GL context was not current inside it\n",
                 rgb[0], rgb[1], rgb[2]);
    return 15;
  }

  std::puts("app_frame_callback_smoke: PASS");
  return 0;
}
