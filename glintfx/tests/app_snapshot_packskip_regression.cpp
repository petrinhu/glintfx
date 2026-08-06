// SPDX-License-Identifier: Apache-2.0
// EN: app_snapshot_packskip_regression (CAPTURE-PACKSKIP, W28, 2026-08-06, auditoria-dominó twin
//     found via App::set_frame_callback's own documented GL-state gap) -- App::snapshot() (app.cpp)
//     is a THIRD, independent, pre-FRAMEGRAB-EMBED implementation of the exact same
//     glReadPixels-into-an-exactly-sized-vector pattern `capture_framebuffer()`
//     (frame_capture.cpp) and `Engine::capture_frame()` (engine.cpp) share -- and was exposed to
//     the SAME heap-buffer-overflow-WRITE bug those two were fixed for (see frame_capture.cpp's
//     own CAPTURE-PACKSKIP comment for the full mechanism). UNIQUE to snapshot() among the three:
//     its own reachable dirty-state source is NOT some caller who happens to share a GL context,
//     it is the HOST'S OWN `frame_cb` hook (App::set_frame_callback) -- `gl_state.hpp`'s own
//     class-level doc comment lists what its GlStateGuard captures around that hook, and none of
//     the eight GL_PACK_*/GL_UNPACK_* pixel-store parameters are among them, so a host's own raw
//     GL calls inside the hook (its own prior glReadPixels/glTexSubImage use, left uncleaned) are
//     NOT reset before `App::snapshot()`'s own glReadPixels runs two lines after `render_frame()`
//     returns.
//
//     PROOF: paint a KNOWN, small, off-corner GREEN rect inside `frame_cb`, immediately followed
//     -- inside the SAME hook invocation, before the render pipeline moves on -- by dirtying
//     GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS and deliberately NOT resetting them (the exact "host
//     forgot to clean up" scenario this fix defends against). `snapshot()` is then called; the
//     PRIMARY assertion is process survival with a well-formed PPM (same "did the process reach
//     this assertion at all" oracle capture_framebuffer_packskip_regression.cpp's own top comment
//     derives -- a corrupted heap aborts inside snapshot()'s own `px` vector destructor, at the
//     latest, long before this test's own read-back code runs). The SECONDARY assertion samples
//     the known green rect's own pixel in the written PPM, the SAME content-correctness bonus
//     app_frame_callback_smoke.cpp's own pattern already established for this fixture.
//     ⚠️ MESA/LLVMPIPE ONLY -- same caveat as every sibling file in this ticket, not re-derived
//     here.
// PT: app_snapshot_packskip_regression (CAPTURE-PACKSKIP, W28, 2026-08-06, gêmeo de
//     auditoria-dominó achado via a própria lacuna de estado GL já documentada do
//     App::set_frame_callback) -- App::snapshot() (app.cpp) é uma TERCEIRA implementação,
//     independente, pré-FRAMEGRAB-EMBED, do exato mesmo padrão
//     glReadPixels-num-vetor-de-tamanho-exato que `capture_framebuffer()` (frame_capture.cpp) e
//     `Engine::capture_frame()` (engine.cpp) compartilham -- e estava exposta ao MESMO bug de
//     ESCRITA heap-buffer-overflow que aquelas duas foram consertadas (ver o próprio comentário
//     CAPTURE-PACKSKIP de frame_capture.cpp pro mecanismo completo). ÚNICO do snapshot() entre os
//     três: a própria fonte alcançável de estado sujo dele NÃO é algum chamador que por acaso
//     compartilha um contexto GL, é o PRÓPRIO hook `frame_cb` do host (App::set_frame_callback) --
//     o próprio comentário de nível de classe de `gl_state.hpp` lista o que o GlStateGuard dele
//     captura ao redor daquele hook, e nenhum dos oito parâmetros de pixel-store GL_PACK_*/
//     GL_UNPACK_* está entre eles, então as próprias chamadas GL cruas do host dentro do hook (o
//     próprio uso anterior de glReadPixels/glTexSubImage dele, deixado sem limpeza) NÃO são
//     resetadas antes do próprio glReadPixels de `App::snapshot()` rodar duas linhas depois do
//     `render_frame()` retornar.
//
//     PROVA: pinta um retângulo VERDE pequeno, num canto conhecido, dentro do `frame_cb`,
//     imediatamente seguido -- dentro da MESMA invocação do hook, antes do pipeline de render
//     seguir adiante -- de sujar GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS e deliberadamente NÃO
//     resetá-los (o exato cenário "o host esqueceu de limpar" contra o qual este conserto
//     defende). O `snapshot()` é então chamado; a asserção PRIMÁRIA é sobrevivência do processo
//     com um PPM bem-formado (o MESMO oráculo "o processo alcançou esta asserção de qualquer
//     jeito" que o próprio comentário de topo de capture_framebuffer_packskip_regression.cpp
//     deriva -- um heap corrompido aborta dentro do próprio destrutor do vetor `px` de
//     snapshot(), o mais tardar, bem antes do próprio código de leitura deste teste rodar). A
//     asserção SECUNDÁRIA amostra o próprio pixel do retângulo verde conhecido no PPM gravado, o
//     MESMO bônus de correção-de-conteúdo que o próprio padrão de app_frame_callback_smoke.cpp já
//     estabeleceu pra esta fixture.
//     ⚠️ SÓ MESA/LLVMPIPE -- mesma ressalva de todo arquivo irmão desta ticket, não re-derivada
//     aqui.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"
#include <cstdio>

int main() {
  glintfx::App app({.title = "app_snapshot_packskip_regression", .width = 300, .height = 200});
  if (!app.ok()) {
    std::puts("FAIL: app ok() false");
    return 1;
  }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); reuses
  //     app_process_event_scene.rml/.rcss, already copied for the app_frame_callback_smoke /
  //     app_capture_frame_smoke block in tests/CMakeLists.txt.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo); reusa
  //     app_process_event_scene.rml/.rcss, já copiado pro bloco app_frame_callback_smoke /
  //     app_capture_frame_smoke em tests/CMakeLists.txt.
  if (!app.load("tests/app_process_event_scene.rml")) {
    std::puts("FAIL: load");
    return 2;
  }
  app.poll_events();
  app.update();

  // EN: Paint a small green rect at window (200,20)..(280,80) -- clear of #btn_a
  //     (10,10)..(90,50) and #scroller (10,60)..(130,160), same disjoint-region discipline
  //     app_capture_frame_smoke.cpp's own geometry note uses. GL bottom-left origin:
  //     gl_y = 200 - 20 - 60 = 120.
  //     THEN, in the SAME hook invocation, dirty GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS and leave
  //     them dirty -- the exact "host forgot to clean up" scenario this fix defends
  //     App::snapshot() against.
  // PT: Pinta um retângulo verde pequeno na janela (200,20)..(280,80) -- livre de #btn_a
  //     (10,10)..(90,50) e #scroller (10,60)..(130,160), mesma disciplina de região disjunta da
  //     própria nota de geometria de app_capture_frame_smoke.cpp. Origem inferior-esquerda do GL:
  //     gl_y = 200 - 20 - 60 = 120.
  //     DEPOIS, na MESMA invocação do hook, suja GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS e os
  //     deixa sujos -- o exato cenário "o host esqueceu de limpar" contra o qual este conserto
  //     defende o App::snapshot().
  app.set_frame_callback([](float) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(200, 120, 80, 60);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // EN: bright green. PT: verde vivo.
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    // EN: THE DIRTY STATE -- the consumer's own exact values (frame_capture.cpp's own
    //     CAPTURE-PACKSKIP comment), left set, never reset by this hook.
    // PT: O ESTADO SUJO -- os próprios valores exatos do consumidor (o próprio comentário
    //     CAPTURE-PACKSKIP de frame_capture.cpp), deixados setados, nunca resetados por este
    //     hook.
    glPixelStorei(GL_PACK_SKIP_PIXELS, 7);
    glPixelStorei(GL_PACK_SKIP_ROWS, 9);
  });

  // EN: THE CALL UNDER TEST. If GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS were still unguarded here,
  //     this is where a corrupted heap would abort the whole process (inside snapshot()'s own
  //     `px` vector destructor, at the latest -- see this file's own top comment).
  // PT: A CHAMADA SOB TESTE. Se GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS ainda estivessem sem
  //     guarda aqui, é aqui que um heap corrompido abortaria o processo inteiro (dentro do
  //     próprio destrutor do vetor `px` de snapshot(), o mais tardar -- ver o próprio comentário
  //     de topo deste arquivo).
  const char* kSnapshotPath = "app_snapshot_packskip_regression.ppm";
  if (!app.snapshot(kSnapshotPath)) {
    std::puts("FAIL: snapshot() returned false");
    return 3;
  }

  FILE* f = std::fopen(kSnapshotPath, "rb");
  if (!f) {
    std::puts("FAIL: could not reopen snapshot PPM");
    return 4;
  }
  char magic[3] = {0};
  int w = 0, h = 0, maxval = 0;
  if (std::fscanf(f, "%2s %d %d %d", magic, &w, &h, &maxval) != 4 || magic[0] != 'P' ||
      magic[1] != '6') {
    std::puts("FAIL: malformed PPM header");
    std::fclose(f);
    return 5;
  }
  std::fgetc(f); // EN: single whitespace byte after the header. PT: um byte de espaço após o cabeçalho.
  if (w != 300 || h != 200) {
    std::fprintf(stderr, "FAIL: snapshot size %dx%d, expected 300x200\n", w, h);
    std::fclose(f);
    return 6;
  }

  // EN: Content-correctness bonus -- PPM is top-left origin (App::snapshot() already flips
  //     glReadPixels' bottom-left rows on write); window (240, 50) is the centre of the hook's
  //     own green rect.
  // PT: Bônus de correção-de-conteúdo -- o PPM é origem superior-esquerda (App::snapshot() já
  //     inverte as linhas bottom-left do glReadPixels ao gravar); a janela (240, 50) é o centro
  //     do próprio retângulo verde do hook.
  const long offset = static_cast<long>(50) * w * 3 + static_cast<long>(240) * 3;
  if (std::fseek(f, offset, SEEK_CUR) != 0) {
    std::puts("FAIL: fseek into pixel data failed");
    std::fclose(f);
    return 7;
  }
  unsigned char rgb[3] = {0, 0, 0};
  if (std::fread(rgb, 1, 3, f) != 3) {
    std::puts("FAIL: short read of pixel data");
    std::fclose(f);
    return 8;
  }
  std::fclose(f);

  std::printf("app_snapshot_packskip_regression: hook rect pixel = (%d, %d, %d)\n", rgb[0], rgb[1],
              rgb[2]);
  if (!(rgb[1] > 180 && rgb[0] < 60 && rgb[2] < 60)) {
    std::fprintf(stderr,
                 "FAIL: hook rect pixel (%d,%d,%d) is not bright green -- snapshot() did not "
                 "capture the expected content despite surviving the dirty GL_PACK_SKIP_* state\n",
                 rgb[0], rgb[1], rgb[2]);
    return 9;
  }

  std::puts(
      "app_snapshot_packskip_regression: PASS (process survived, PPM well-formed, hook "
      "rect content correct)");
  return 0;
}
