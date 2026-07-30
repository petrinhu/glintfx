// SPDX-License-Identifier: MPL-2.0
// EN: `FRAMEGRAB-TEX` -- verifies App::capture_frame(): the composited-frame-to-memory sibling
//     of App::snapshot() (app.hpp, right before `private:`). The load-bearing question this
//     test answers is NOT "does it return true" -- a stale/garbage/wrong-surface read can
//     easily return ok()==true with meaningless pixels. It is: **does the returned buffer
//     contain the FULLY COMPOSITED frame** (scene-hook content AND RmlUi UI content, BOTH),
//     not the RmlUi internal offscreen layer the item's own investigation found `frame_callback`
//     runs against (engine.cpp, `render.begin_frame()`..`render.end_frame()`), and not some
//     stale/previous frame.
//
//     ORACLE (three known, NON-OVERLAPPING colours in one captured buffer, per the item's own
//     suggested method): reuses app_process_event_scene.rml/.rcss (body #101010 dark background,
//     #btn_a #ff0000 red at window (10,10)..(90,50), #scroller at (10,60)..(130,160)) --
//     app_frame_callback_smoke.cpp already proved the hook-vs-UI ORDERING (hook underneath,
//     RmlUi's own content composited on top) with an OVERLAPPING paint; this test instead
//     samples THREE DISJOINT regions to prove all three layers of the composite reach the
//     returned buffer simultaneously:
//       (A) the frame_callback hook paints solid BLUE into window rect (200,120)..(280,180) --
//           chosen clear of #btn_a AND #scroller (see the geometry note below) -- proving the
//           hook's own draw (the "game scene" a real consumer freezes) is present;
//       (B) #btn_a's own RED at its centre (50,30) -- proving RmlUi's UI content composited on
//           top, not just the hook's raw output;
//       (C) plain background DARK #101010 at (150,100) -- clear of A, B, and #scroller --
//           proving the read is not simply returning a solid/garbage fill that happens to
//           satisfy A and B by accident (e.g. an uninitialised buffer read as noise could,
//           in principle, coincidentally hit blue/red at two sample points; hitting the exact
//           THIRD, distinct expected colour at a third point makes that vanishingly unlikely).
//     Row 0 of CapturedFrame::pixels is documented TOP-of-image (app.hpp's own CapturedFrame
//     doc-comment) -- the SAME orientation as window-space (x, y), so sample points map
//     directly: pixel index = (y * width + x) * 4, no row-flip needed by this test (unlike
//     app_frame_callback_smoke.cpp's PPM reader, which has to seek because PPM is ALSO top-down
//     but glReadPixels itself is bottom-up -- capture_frame() does that flip internally, this
//     test is the proof the flip lands the pixel at the coordinate a caller actually expects).
//
//     GEOMETRY: window is 300x200 (matches every other app_process_event_scene.rml consumer).
//     Hook rect window (200,120)..(280,180), 80x60 -- x-range [200,280) is entirely to the
//     RIGHT of #scroller's x-range [10,130) and #btn_a's x-range [10,90), so no overlap on
//     either axis with either fixture element. GL's glScissor is bottom-left-origin (the SAME
//     conversion app_frame_callback_smoke.cpp's own hook uses): gl_y = window_height - y - h =
//     200 - 120 - 60 = 20.
//
//     MUTATION PROOF (required by this session's own discipline: a test that passes by
//     accident is not a test): see this file's own git history / the FRAMEGRAB-TEX commit notes
//     for the sabotage-rebuild-confirm-red-restore-rebuild-confirm-green cycle this test was
//     run through before being trusted -- reordering capture_frame()'s glReadPixels call to
//     AFTER window.swap() (the exact AUD-PUB-1-TWIN class of bug snapshot() itself was once
//     fixed for) turns this test red under Mesa/llvmpipe+Xvfb, because the back-buffer's
//     content is undefined post-swap on that driver stack (app.cpp's own snapshot() doc-comment
//     already states this as the reason its OWN read happens pre-swap).
// PT: `FRAMEGRAB-TEX` -- verifica App::capture_frame(): o irmão do App::snapshot() (app.hpp,
//     logo antes do `private:`) que devolve o frame composto para memória. A pergunta que
//     carrega peso aqui NÃO é "devolve true" -- uma leitura de superfície errada/frame
//     obsoleto/lixo pode facilmente devolver ok()==true com pixels sem sentido. É: **o buffer
//     devolvido contém o frame TOTALMENTE COMPOSTO** (conteúdo do hook de cena E conteúdo de UI
//     do RmlUi, OS DOIS), não a layer offscreen interna do RmlUi contra a qual a própria
//     investigação do item achou que o `frame_callback` roda (engine.cpp, entre
//     `render.begin_frame()` e `render.end_frame()`), e não algum frame obsoleto/anterior.
//
//     ORÁCULO (três cores conhecidas, NÃO-sobrepostas, num único buffer capturado, pelo método
//     que o próprio item sugeriu): reusa app_process_event_scene.rml/.rcss (fundo escuro do body
//     #101010, #btn_a #ff0000 vermelho na janela (10,10)..(90,50), #scroller em
//     (10,60)..(130,160)) -- o app_frame_callback_smoke.cpp já provou a ORDEM hook-vs-UI (hook
//     por baixo, conteúdo próprio do RmlUi composto por cima) com uma pintura SOBREPOSTA; este
//     teste em vez disso amostra TRÊS regiões DISJUNTAS pra provar que as três camadas do
//     composto chegam ao buffer devolvido simultaneamente:
//       (A) o hook frame_callback pinta AZUL sólido no retângulo de janela (200,120)..(280,180)
//           -- escolhido livre de #btn_a E #scroller (ver a nota de geometria abaixo) --
//           provando que o próprio desenho do hook (a "cena do jogo" que um consumidor real
//           congela) está presente;
//       (B) o próprio VERMELHO de #btn_a no centro dele (50,30) -- provando que o conteúdo de UI
//           do RmlUi compôs por cima, não só a saída crua do hook;
//       (C) fundo escuro simples #101010 em (150,100) -- livre de A, B e #scroller -- provando
//           que a leitura não está simplesmente devolvendo um preenchimento sólido/lixo que por
//           acaso satisfaz A e B (ex.: um buffer não-inicializado lido como ruído poderia, em
//           princípio, bater coincidentemente com azul/vermelho em dois pontos de amostra;
//           acertar a TERCEIRA cor esperada, distinta, num terceiro ponto torna isso
//           extremamente improvável).
//     A linha 0 de CapturedFrame::pixels é documentada como o TOPO da imagem (o próprio
//     doc-comment de CapturedFrame em app.hpp) -- a MESMA orientação do espaço-janela (x, y),
//     então os pontos de amostra mapeiam direto: índice de pixel = (y * width + x) * 4, sem
//     inversão de linha necessária por este teste (diferente do leitor de PPM do
//     app_frame_callback_smoke.cpp, que precisa de seek porque o PPM TAMBÉM é top-down mas o
//     próprio glReadPixels é bottom-up -- capture_frame() já faz essa inversão internamente;
//     este teste é a prova de que a inversão põe o pixel na coordenada que um chamador de fato
//     espera).
//
//     GEOMETRIA: a janela é 300x200 (bate com todo outro consumidor de
//     app_process_event_scene.rml). Retângulo do hook, janela (200,120)..(280,180), 80x60 -- a
//     faixa-x [200,280) fica inteiramente à DIREITA da faixa-x de #scroller [10,130) e de #btn_a
//     [10,90), então sem sobreposição em nenhum eixo com nenhum elemento da fixture. O
//     glScissor do GL tem origem inferior-esquerda (a MESMA conversão que o próprio hook de
//     app_frame_callback_smoke.cpp usa): gl_y = altura_da_janela - y - h = 200 - 120 - 60 = 20.
//
//     PROVA DE MUTAÇÃO (exigida pela própria disciplina desta sessão: um teste que passa por
//     acidente não é teste): ver o histórico git deste arquivo / as notas de commit do
//     FRAMEGRAB-TEX pro ciclo sabota-rebuild-confirma-vermelho-restaura-rebuild-confirma-verde
//     por que este teste passou antes de ser confiado -- reordenar a chamada glReadPixels de
//     capture_frame() pra DEPOIS de window.swap() (a MESMA classe de bug AUD-PUB-1-TWIN que o
//     próprio snapshot() já corrigiu uma vez) deixa este teste vermelho sob Mesa/llvmpipe+Xvfb,
//     porque o conteúdo do back-buffer é indefinido pós-swap nessa pilha de driver (o próprio
//     doc-comment de snapshot() em app.cpp já declara isto como o motivo da PRÓPRIA leitura dele
//     acontecer pré-swap).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"
#include <cstdio>
#include <cstdlib>

namespace {

// EN: Loose thresholds tolerant of llvmpipe AA/blend at the rect edge -- every sample point
//     below is deep in the interior of its region, so the margin is generous defence-in-depth,
//     not a requirement (same discipline as app_frame_callback_smoke.cpp's own thresholds).
// PT: Limiares frouxos, tolerantes a AA/blend do llvmpipe na borda do retângulo -- todo ponto de
//     amostra abaixo está fundo no interior da própria região, então a margem é defesa-em-
//     profundidade generosa, não um requisito (mesma disciplina dos próprios limiares de
//     app_frame_callback_smoke.cpp).
bool looks_blue(unsigned char r, unsigned char g, unsigned char b) {
  return b > 180 && r < 60 && g < 60;
}
bool looks_red(unsigned char r, unsigned char g, unsigned char b) {
  return r > 180 && g < 60 && b < 60;
}
bool looks_dark_bg(unsigned char r, unsigned char g, unsigned char b) {
  // EN: body { background-color: #101010; } == rgb(16,16,16) -- allow a generous +/-24 band.
  // PT: body { background-color: #101010; } == rgb(16,16,16) -- permite uma banda generosa de +/-24.
  return r < 40 && g < 40 && b < 40;
}

} // namespace

int main() {
  glintfx::App app({.title = "app_capture_frame_smoke", .width = 300, .height = 200});
  if (!app.ok()) {
    std::puts("FAIL: app ok() false");
    return 1;
  }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); reuses
  //     app_process_event_scene.rml/.rcss (already copied for app_process_event_smoke /
  //     app_frame_callback_smoke, same tests/CMakeLists.txt block).
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo); reusa
  //     app_process_event_scene.rml/.rcss (já copiado pra app_process_event_smoke /
  //     app_frame_callback_smoke, mesmo bloco de tests/CMakeLists.txt).
  if (!app.load("tests/app_process_event_scene.rml")) {
    std::puts("FAIL: load");
    return 2;
  }

  // EN: One ordinary poll/update/render cycle FIRST -- the same poll_events() -> update() ->
  //     render() pattern every other App test uses before its first snapshot()/pixel assertion
  //     (app_smoke.cpp, app_dp_ratio_smoke.cpp). Context::Update() computes the document's
  //     layout; skipping it (this test's own first draft did, and caught itself) leaves #btn_a
  //     un-laid-out and Context::Render() draws nothing for it, which would have made this
  //     test's own (B) sample point a FALSE FAIL indistinguishable from a real capture_frame()
  //     bug -- worth naming so a future reader does not "fix" capture_frame() to chase this.
  // PT: Um ciclo comum de poll/update/render PRIMEIRO -- o MESMO padrão poll_events() ->
  //     update() -> render() que todo outro teste de App usa antes da própria primeira
  //     asserção de snapshot()/pixel (app_smoke.cpp, app_dp_ratio_smoke.cpp). O
  //     Context::Update() computa o layout do documento; pulá-lo (o primeiro rascunho deste
  //     teste pulou, e se pegou sozinho) deixa #btn_a sem layout e o Context::Render() não
  //     desenha nada pra ele, o que teria tornado o próprio ponto de amostra (B) deste teste um
  //     FALSO FAIL indistinguível de um bug real do capture_frame() -- vale nomear pra um leitor
  //     futuro não "consertar" o capture_frame() perseguindo isto.
  app.poll_events();
  app.update();
  app.render();

  // EN: (A) hook paints solid blue into a rect clear of #btn_a and #scroller -- see file header
  //     for the geometry derivation.
  // PT: (A) o hook pinta azul sólido num retângulo livre de #btn_a e #scroller -- ver o
  //     cabeçalho do arquivo pra derivação da geometria.
  app.set_frame_callback([](float) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(200, 20, 80, 60);           // EN: window (200,120)..(280,180), GL bottom-left origin.
                                          // PT: janela (200,120)..(280,180), origem inferior-esquerda do GL.
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // EN: solid blue. PT: azul sólido.
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
  });

  const glintfx::App::CapturedFrame frame = app.capture_frame();
  if (!frame.ok) {
    std::puts("FAIL: capture_frame() returned ok == false");
    return 3;
  }
  if (frame.width != 300 || frame.height != 200) {
    std::fprintf(stderr, "FAIL: capture_frame() size %dx%d, expected 300x200\n",
                 frame.width, frame.height);
    return 4;
  }
  const std::size_t expected_bytes =
      static_cast<std::size_t>(frame.width) * static_cast<std::size_t>(frame.height) * 4;
  if (frame.byte_count != expected_bytes) {
    std::fprintf(stderr, "FAIL: byte_count=%zu, expected %zu\n", frame.byte_count, expected_bytes);
    return 5;
  }
  if (!frame.pixels) {
    std::puts("FAIL: pixels is null despite ok == true");
    return 6;
  }

  auto sample = [&](int x, int y, unsigned char out[4]) {
    const std::size_t idx =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width) +
         static_cast<std::size_t>(x)) *
        4;
    out[0] = frame.pixels[idx + 0];
    out[1] = frame.pixels[idx + 1];
    out[2] = frame.pixels[idx + 2];
    out[3] = frame.pixels[idx + 3];
  };

  // EN: (A) hook's blue, centre of the (200,120)..(280,180) rect: (240, 150).
  // PT: (A) azul do hook, centro do retângulo (200,120)..(280,180): (240, 150).
  unsigned char hook_px[4];
  sample(240, 150, hook_px);
  std::printf("app_capture_frame_smoke: hook rect pixel = (%d, %d, %d, %d)\n",
              hook_px[0], hook_px[1], hook_px[2], hook_px[3]);
  if (!looks_blue(hook_px[0], hook_px[1], hook_px[2])) {
    std::fprintf(stderr,
                 "FAIL: hook rect pixel (%d,%d,%d) is not blue -- capture_frame() did not "
                 "capture the scene-hook content (wrong surface, or read before the hook ran)\n",
                 hook_px[0], hook_px[1], hook_px[2]);
    return 7;
  }
  if (hook_px[3] != 255) {
    std::fprintf(stderr, "FAIL: hook rect alpha=%d, expected 255 (synthetic full opacity)\n",
                 hook_px[3]);
    return 8;
  }

  // EN: (B) #btn_a's own red, its centre: (50, 30).
  // PT: (B) o próprio vermelho de #btn_a, o centro dele: (50, 30).
  unsigned char btn_px[4];
  sample(50, 30, btn_px);
  std::printf("app_capture_frame_smoke: btn_a centre pixel = (%d, %d, %d, %d)\n",
              btn_px[0], btn_px[1], btn_px[2], btn_px[3]);
  if (!looks_red(btn_px[0], btn_px[1], btn_px[2])) {
    std::fprintf(stderr,
                 "FAIL: btn_a centre pixel (%d,%d,%d) is not red -- capture_frame() did not "
                 "capture RmlUi's own UI content (offscreen layer read, or composited-under "
                 "the hook instead of over it)\n",
                 btn_px[0], btn_px[1], btn_px[2]);
    return 9;
  }
  if (btn_px[3] != 255) {
    std::fprintf(stderr, "FAIL: btn_a alpha=%d, expected 255 (synthetic full opacity)\n",
                 btn_px[3]);
    return 10;
  }

  // EN: (C) plain body background, clear of A, B and #scroller: (150, 100).
  // PT: (C) fundo simples do body, livre de A, B e #scroller: (150, 100).
  unsigned char bg_px[4];
  sample(150, 100, bg_px);
  std::printf("app_capture_frame_smoke: background pixel = (%d, %d, %d, %d)\n",
              bg_px[0], bg_px[1], bg_px[2], bg_px[3]);
  if (!looks_dark_bg(bg_px[0], bg_px[1], bg_px[2])) {
    std::fprintf(stderr,
                 "FAIL: background pixel (%d,%d,%d) is not the expected dark #101010 -- "
                 "suggests a garbage/uninitialised/solid-fill read rather than a real composite\n",
                 bg_px[0], bg_px[1], bg_px[2]);
    return 11;
  }
  if (bg_px[3] != 255) {
    std::fprintf(stderr, "FAIL: background alpha=%d, expected 255 (synthetic full opacity)\n",
                 bg_px[3]);
    return 12;
  }

  std::puts("app_capture_frame_smoke: PASS");
  return 0;
}
