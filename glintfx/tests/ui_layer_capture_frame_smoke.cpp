// SPDX-License-Identifier: MPL-2.0
// EN: `FRAMEGRAB-EMBED` -- verifies UiLayer::capture_frame(): the embed-mode sibling of
//     App::capture_frame() (app_capture_frame_smoke.cpp), reading the SAME shared
//     Engine::capture_frame readback (engine.cpp) both facades wrap. This file exercises the
//     load-bearing question app_capture_frame_smoke.cpp already established for App -- **does
//     the returned buffer contain the FULLY COMPOSITED frame** (host scene AND glintfx UI
//     content, BOTH, not a stale/garbage/wrong-surface read) -- PLUS the parts that are
//     genuinely NEW for the embed path per the FRAMEGRAB-EMBED contract (ui_layer.hpp, right
//     before `private:`): the declared REGION ceiling (letterbox sub-viewport, not the whole
//     window), the ODD-WIDTH class of bug the review flagged (GL_PACK_ALIGNMENT corrupting
//     rows past the first when the default 4-byte alignment doesn't match a tightly-packed
//     3-bytes-per-pixel RGB row), calling BEFORE load() (capture_frame() needs no document),
//     and the zero-width DEGENERATE-SIZE guard (Part G, added after the team-lead's own
//     follow-up on the FRAMEGRAB-EMBED thread, 2026-07-30 -- see that part's own doc-comment
//     for the full derivation of why `set_viewport()` cannot inject it and `UiLayerConfig`
//     can).
//
//     ORACLE, part A (composite correctness): min_partial.rml (already used by
//     ui_layer_compose.cpp) puts a solid white #box at window (10,10)..(90,50), transparent
//     body elsewhere -- reused here so a compose-only anchor-colour proof and a UI-content
//     proof share one fixture. THREE disjoint sample points, mirroring
//     app_capture_frame_smoke.cpp's own three-colour method:
//       (A) a host-drawn BLUE rect at window (200,120)..(280,180) -- painted directly into
//           FBO 0 by this test BEFORE calling render(), the SAME geometry
//           app_capture_frame_smoke.cpp's own frame_callback hook uses (clear of #box on
//           both axes) -- proves the host's OWN pre-render() drawing survives the compose-only
//           UI overlay AND reaches the captured buffer;
//       (B) #box's own WHITE at its centre (50,30) -- proves the glintfx UI content
//           composited on top is captured, not just the host's raw draw;
//       (C) the plain host anchor colour, clear of A and B, at (150,100) -- proves the read
//           is a real composite, not a solid/garbage fill that happens to satisfy A and B by
//           accident (same reasoning as the App-side test's own (C) sample).
//     Row 0 of CapturedFrame::pixels is documented TOP-of-image (ui_layer.hpp's own
//     CapturedFrame doc-comment, mirroring App::CapturedFrame's) -- the SAME orientation as
//     window-space (x, y), so sample points map directly: pixel index = (y*width+x)*4.
//
//     Part B (capture does not corrupt a SUBSEQUENT render): re-clears FBO 0 to a DIFFERENT
//     anchor colour, renders + captures a SECOND time, and re-checks the SAME three regions --
//     proves capture_frame()'s own GL state save/restore (engine.hpp's own
//     GL_PACK_*/GL_READ_FRAMEBUFFER contract) does not leave the context in a state that
//     breaks a following render()/capture_frame() pair.
//
//     Part C (capture before load()): a FRESH UiLayer, load() never called, host FBO 0
//     cleared to a known colour, capture_frame() called with NEITHER update() NOR render()
//     run first -- proves the call needs no loaded document (it reads FBO 0 as-is, per its own
//     doc-comment) and returns exactly the host's own clear colour.
//
//     EDGE CASES CONSIDERED AND REJECTED (measured, not assumed -- read this before adding
//     either back): the review that requested this fatia also asked for a "moved-from
//     UiLayer" and a "no current GL context" case. Both were WRITTEN, RUN, and REMOVED:
//       - "moved-from": `UiLayer b = std::move(a); a.capture_frame();` SEGFAULTED
//         (`impl_` is null after a move -- confirmed by gdb: `capture_frame() const`
//         dereferences `impl_->ok` without checking `impl_` first). This is NOT a
//         capture_frame()-specific bug: ui_layer.hpp's own class-level doc-comment states
//         "Calling ANY method on `a` other than ok() or the destructor is undefined
//         behaviour -- same convention as std::unique_ptr", and EVERY pre-existing UiLayer
//         method (get_element_box, scroll_element_into_view, all ~30 of them) has the exact
//         same `if (!impl_->ok)` shape with no `impl_ &&` guard -- capture_frame() simply
//         matches the established, documented convention. Testing it would be asserting
//         behaviour on documented UB, which is not a sound test. Flagged to the team lead as
//         a possible class-wide hardening follow-up (`impl_ &&` on every method, not just
//         this new one), not fixed unilaterally here -- out of this fatia's scope.
//       - "no current GL context": section 26 of docs/embed-integration.md ("App-mode raw-GL
//         cohabitation") already states, for the exact same class of ordering mistake, that
//         "glintfx adds no guard of its own against this specific ordering mistake" and that
//         the failure mode is driver-dependent and NOT glintfx's own code to guarantee --
//         glGetIntegerv/glReadPixels/etc. called with no context current is undefined at the
//         GL specification level, not something this function's own logic can make
//         deterministic. A test asserting a specific outcome here would be testing the
//         driver, not the contract.
// PT: CASOS-LIMITE CONSIDERADOS E REJEITADOS (medidos, não presumidos -- leia isto antes de
//     adicionar qualquer um de volta): o review que pediu esta fatia também pediu um caso de
//     "UiLayer movido-de" e um de "sem contexto GL corrente". Os dois foram ESCRITOS,
//     RODADOS, e REMOVIDOS:
//       - "movido-de": `UiLayer b = std::move(a); a.capture_frame();` deu SEGFAULT (`impl_`
//         é nulo após um move -- confirmado por gdb: `capture_frame() const` desreferencia
//         `impl_->ok` sem checar `impl_` primeiro). Isto NÃO é um bug específico do
//         capture_frame(): o próprio comentário de nível de classe de ui_layer.hpp declara
//         "Chamar QUALQUER método em `a` além de ok() ou o destrutor é comportamento
//         indefinido -- mesma convenção do std::unique_ptr", e TODO método pré-existente do
//         UiLayer (get_element_box, scroll_element_into_view, os ~30 deles) tem exatamente o
//         mesmo formato `if (!impl_->ok)` sem guard `impl_ &&` -- o capture_frame() só
//         segue a convenção estabelecida e documentada. Testá-lo seria afirmar
//         comportamento sobre UB documentado, o que não é um teste sólido. Sinalizado ao
//         líder do time como possível desdobramento de hardening em nível de classe
//         (`impl_ &&` em todo método, não só este novo), não consertado unilateralmente
//         aqui -- fora do escopo desta fatia.
//       - "sem contexto GL corrente": a seção 26 de docs/embed-integration.md ("Contrato de
//         coabitação GL cru no modo App") já declara, para a exata mesma classe de erro de
//         ordenação, que "a glintfx não acrescenta guarda própria contra este erro
//         específico de ordenação" e que o modo de falha depende do driver e NÃO é código da
//         própria glintfx a garantir -- glGetIntegerv/glReadPixels/etc. chamados sem
//         contexto corrente é indefinido no nível da especificação GL, não algo que a
//         própria lógica desta função consiga tornar determinístico. Um teste afirmando um
//         resultado específico aqui estaria testando o driver, não o contrato.
//
//     Part E (letterbox region -- the declared ceiling): a LARGER host window (400x300)
//     cleared entirely to one anchor colour, a UiLayer configured via the 5-arg
//     set_viewport(x=50, y=30, w=200, h=150, target_h=300) sub-region, with min_partial.rml
//     loaded inside it. Asserts frame.width==200 && frame.height==150 (NOT 400x300) -- proof
//     the capture is cropped to the CURRENT viewport, not the whole render target -- and
//     re-samples #box's own local centre (50,30) inside the returned (smaller) buffer, which
//     is only white if the GL-native offset conversion (gl_offset_x/y, ui_layer.cpp) reads the
//     CORRECT rectangle of the larger window, not an arbitrarily-shifted one.
//
//     Part F (odd width -- the GL_PACK_ALIGNMENT regression class): a 301x200 host window
//     (ODD width, no document loaded) filled with one solid colour. Samples BOTH corners AND
//     several interior/edge points across MULTIPLE rows (not just row 0) -- the failure mode
//     this guards against (default GL_PACK_ALIGNMENT=4 on a tightly-packed 3-bytes/pixel RGB
//     read) corrupts rows AFTER the first, so a corner-only or single-row check would miss it.
//
//     MUTATION PROOF (this session's own discipline: a test that passes by accident is not a
//     test): during FRAMEGRAB-EMBED implementation, Engine::capture_frame's forced
//     `glPixelStorei(GL_PACK_ALIGNMENT, 1)` was temporarily removed (falling back to whatever
//     alignment the caller/driver defaults to) and this file's Part F was rebuilt and
//     re-run -- it went RED (corrupted rows past the first, exactly the predicted class of
//     failure). Restoring the forced alignment=1 and rebuilding turned it back GREEN. See the
//     FRAMEGRAB-EMBED commit notes for the full sabotage-rebuild-confirm-red-restore-rebuild-
//     confirm-green cycle.
// PT: `FRAMEGRAB-EMBED` -- verifica UiLayer::capture_frame(): o irmão em modo embed do
//     App::capture_frame() (app_capture_frame_smoke.cpp), lendo o MESMO readback
//     compartilhado Engine::capture_frame (engine.cpp) que as duas fachadas encapsulam. Este
//     arquivo exercita a pergunta que carrega peso que app_capture_frame_smoke.cpp já
//     estabeleceu para o App -- **o buffer devolvido contém o frame TOTALMENTE COMPOSTO**
//     (cena do host E conteúdo de UI da glintfx, OS DOIS, não uma leitura obsoleta/lixo/de
//     superfície errada) -- MAIS as partes que são genuinamente NOVAS pro caminho embed
//     conforme o contrato do FRAMEGRAB-EMBED (ui_layer.hpp, logo antes de `private:`): o teto
//     de REGIÃO declarado (sub-viewport de letterbox, não a janela inteira), a classe de bug
//     de LARGURA ÍMPAR que o review sinalizou (GL_PACK_ALIGNMENT corrompendo linhas após a
//     primeira quando o alinhamento default de 4 bytes não bate com uma linha RGB
//     compactada de 3 bytes-por-pixel), chamar ANTES de load() (capture_frame() não precisa
//     de documento), e o guard de TAMANHO DEGENERADO de largura zero (Parte G, somada após o
//     próprio desdobramento do team-lead na thread do FRAMEGRAB-EMBED, 2026-07-30 -- ver o
//     próprio doc-comment dessa parte pra derivação completa de por que `set_viewport()` não
//     consegue injetar isto e o `UiLayerConfig` consegue).
//
//     ORÁCULO, parte A (correção do composto): min_partial.rml (já usado por
//     ui_layer_compose.cpp) põe um #box branco sólido na janela (10,10)..(90,50), body
//     transparente no resto -- reusado aqui para que uma prova de cor-âncora compose-only e
//     uma prova de conteúdo-UI compartilhem uma fixture. TRÊS pontos de amostra disjuntos,
//     espelhando o próprio método de três cores de app_capture_frame_smoke.cpp:
//       (A) um retângulo AZUL desenhado pelo host na janela (200,120)..(280,180) -- pintado
//           direto no FBO 0 por este teste ANTES de chamar render(), a MESMA geometria que o
//           próprio hook frame_callback de app_capture_frame_smoke.cpp usa (livre do #box nos
//           dois eixos) -- prova que o próprio desenho pré-render() do host sobrevive à
//           sobreposição compose-only da UI E alcança o buffer capturado;
//       (B) o próprio BRANCO do #box no centro dele (50,30) -- prova que o conteúdo de UI da
//           glintfx composto por cima é capturado, não só o desenho cru do host;
//       (C) a cor-âncora simples do host, livre de A e B, em (150,100) -- prova que a leitura
//           é um composto real, não um preenchimento sólido/lixo que por acaso satisfaz A e B
//           por acidente (mesma racional do próprio ponto de amostra (C) do teste do App).
//     A linha 0 de CapturedFrame::pixels é documentada como o TOPO da imagem (o próprio
//     doc-comment de CapturedFrame em ui_layer.hpp, espelhando o de App::CapturedFrame) -- a
//     MESMA orientação do espaço-janela (x, y), então os pontos de amostra mapeiam direto:
//     índice de pixel = (y*width+x)*4.
//
//     Parte B (a captura não corrompe um render SUBSEQUENTE): relimpa o FBO 0 com uma cor-
//     âncora DIFERENTE, renderiza + captura uma SEGUNDA vez, e reconfere as MESMAS três
//     regiões -- prova que o próprio salvar/restaurar estado GL de capture_frame() (o próprio
//     contrato GL_PACK_*/GL_READ_FRAMEBUFFER de engine.hpp) não deixa o contexto num estado
//     que quebra um par render()/capture_frame() seguinte.
//
//     Parte C (captura antes de load()): um UiLayer NOVO, load() nunca chamado, FBO 0 do host
//     limpo com uma cor conhecida, capture_frame() chamado sem update() NEM render() rodarem
//     antes -- prova que a chamada não precisa de documento carregado (lê o FBO 0 como está,
//     conforme o próprio doc-comment) e devolve exatamente a própria cor de clear do host.
//
//     Parte E (região de letterbox -- o teto declarado): uma janela do host MAIOR (400x300)
//     limpa inteira com uma cor-âncora, um UiLayer configurado via a sobrecarga de 5 args
//     set_viewport(x=50, y=30, w=200, h=150, target_h=300) de sub-região, com min_partial.rml
//     carregado dentro dela. Afirma frame.width==200 && frame.height==150 (NÃO 400x300) --
//     prova de que a captura é recortada ao viewport CORRENTE, não ao render target inteiro --
//     e reamostra o próprio centro local do #box (50,30) dentro do buffer devolvido (menor),
//     que só é branco se a conversão de offset nativo-GL (gl_offset_x/y, ui_layer.cpp) ler o
//     retângulo CORRETO da janela maior, não um arbitrariamente deslocado.
//
//     Parte F (largura ímpar -- a classe de regressão do GL_PACK_ALIGNMENT): uma janela do
//     host 301x200 (largura ÍMPAR, nenhum documento carregado) preenchida com uma cor sólida.
//     Amostra AMBOS os cantos E vários pontos internos/de borda através de MÚLTIPLAS linhas
//     (não só a linha 0) -- o modo de falha contra o qual isto se defende (GL_PACK_ALIGNMENT
//     default=4 numa leitura RGB compactada de 3 bytes/pixel) corrompe linhas APÓS a
//     primeira, então uma checagem só-de-canto ou só-de-uma-linha o perderia.
//
//     PROVA DE MUTAÇÃO (disciplina desta sessão: um teste que passa por acidente não é
//     teste): durante a implementação do FRAMEGRAB-EMBED, o `glPixelStorei(GL_PACK_ALIGNMENT,
//     1)` forçado de Engine::capture_frame foi temporariamente removido (caindo no
//     alinhamento default que o chamador/driver já tivesse) e a Parte F deste arquivo foi
//     rebuildada e re-rodada -- ficou VERMELHA (linhas corrompidas após a primeira,
//     exatamente a classe de falha prevista). Restaurar o alinhamento=1 forçado e rebuildar a
//     trouxe de volta a VERDE. Ver as notas de commit do FRAMEGRAB-EMBED pro ciclo completo
//     sabota-rebuild-confirma-vermelho-restaura-rebuild-confirma-verde.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h. PT: inclui gl_loader.h.
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace {

bool looks_like(unsigned char r, unsigned char g, unsigned char b,
                int er, int eg, int eb, int tol = 8) {
  return std::abs(int(r) - er) <= tol && std::abs(int(g) - eg) <= tol &&
         std::abs(int(b) - eb) <= tol;
}

void sample(const glintfx::UiLayer::CapturedFrame& frame, int x, int y, unsigned char out[4]) {
  const std::size_t idx =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width) +
       static_cast<std::size_t>(x)) *
      4;
  out[0] = frame.pixels[idx + 0];
  out[1] = frame.pixels[idx + 1];
  out[2] = frame.pixels[idx + 2];
  out[3] = frame.pixels[idx + 3];
}

// EN: Paint the host's own "game scene" rect directly into FBO 0 -- same geometry
//     app_capture_frame_smoke.cpp's frame_callback hook uses (window (200,120)..(280,180),
//     clear of min_partial.rml's #box at (10,10)..(90,50)). GL's glScissor is bottom-left
//     origin: gl_y = window_height - y - h.
// PT: Pinta o próprio retângulo de "cena de jogo" do host direto no FBO 0 -- mesma geometria
//     que o hook frame_callback de app_capture_frame_smoke.cpp usa (janela
//     (200,120)..(280,180), livre do #box de min_partial.rml em (10,10)..(90,50)). O
//     glScissor do GL tem origem inferior-esquerda: gl_y = altura_da_janela - y - h.
void paint_host_scene_rect(int window_h) {
  glEnable(GL_SCISSOR_TEST);
  glScissor(200, window_h - 120 - 60, 80, 60);
  glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // solid blue
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

bool part_a_and_b(glintfx::WindowGlfw& host, glintfx::UiLayer& ui) {
  const int W = 300, H = 200;

  auto one_pass = [&](float anchor_r, float anchor_g, float anchor_b) -> bool {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(anchor_r, anchor_g, anchor_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    paint_host_scene_rect(H);

    ui.update();
    ui.render();

    const glintfx::UiLayer::CapturedFrame frame = ui.capture_frame();
    if (!frame.ok) {
      std::puts("FAIL: capture_frame() ok == false");
      return false;
    }
    if (frame.width != W || frame.height != H) {
      std::fprintf(stderr, "FAIL: size %dx%d, expected %dx%d\n", frame.width, frame.height, W, H);
      return false;
    }
    const std::size_t expected_bytes =
        static_cast<std::size_t>(W) * static_cast<std::size_t>(H) * 4;
    if (frame.byte_count != expected_bytes) {
      std::fprintf(stderr, "FAIL: byte_count=%zu expected %zu\n", frame.byte_count, expected_bytes);
      return false;
    }
    if (!frame.pixels) {
      std::puts("FAIL: pixels null despite ok");
      return false;
    }

    unsigned char px[4];
    sample(frame, 240, 150, px);
    if (!looks_like(px[0], px[1], px[2], 0, 0, 255)) {
      std::fprintf(stderr, "FAIL: host-scene sample (%d,%d,%d) not blue\n", px[0], px[1], px[2]);
      return false;
    }
    if (px[3] != 255) {
      std::puts("FAIL: host-scene alpha != 255");
      return false;
    }

    sample(frame, 50, 30, px);
    if (!looks_like(px[0], px[1], px[2], 255, 255, 255)) {
      std::fprintf(stderr, "FAIL: #box sample (%d,%d,%d) not white\n", px[0], px[1], px[2]);
      return false;
    }
    if (px[3] != 255) {
      std::puts("FAIL: #box alpha != 255");
      return false;
    }

    sample(frame, 150, 100, px);
    const int er = int(anchor_r * 255.f), eg = int(anchor_g * 255.f), eb = int(anchor_b * 255.f);
    if (!looks_like(px[0], px[1], px[2], er, eg, eb)) {
      std::fprintf(stderr, "FAIL: bg sample (%d,%d,%d) not anchor (%d,%d,%d)\n",
                   px[0], px[1], px[2], er, eg, eb);
      return false;
    }
    if (px[3] != 255) {
      std::puts("FAIL: bg alpha != 255");
      return false;
    }
    return true;
  };

  // EN: Part A -- first pass, anchor #101010 (mirrors app_capture_frame_smoke.cpp's own bg).
  // PT: Parte A -- primeira passada, âncora #101010 (espelha o próprio bg de
  //     app_capture_frame_smoke.cpp).
  if (!one_pass(16.f / 255.f, 16.f / 255.f, 16.f / 255.f)) return false;

  // EN: Part B -- second pass, DIFFERENT anchor, proves capture_frame() left no GL state
  //     behind that would corrupt a following render()/capture_frame() pair.
  // PT: Parte B -- segunda passada, âncora DIFERENTE, prova que capture_frame() não deixou
  //     estado GL para trás que corromperia um par render()/capture_frame() seguinte.
  if (!one_pass(30.f / 255.f, 40.f / 255.f, 50.f / 255.f)) return false;

  (void)host;
  std::puts("ui_layer_capture_frame_smoke: parts A/B PASS");
  return true;
}

bool part_c_before_load() {
  glintfx::WindowGlfw host;
  if (!host.create("capture_before_load_host", 120, 80)) {
    std::puts("FAIL: part C host create");
    return false;
  }
  glintfx::UiLayer ui({.logical_width = 120, .logical_height = 80, .load_gl = true});
  if (!ui.ok()) {
    std::puts("FAIL: part C ui attach");
    return false;
  }
  // EN: Deliberately NEVER call load(), update(), or render() -- capture_frame() must still
  //     work, reading whatever the host itself put in FBO 0.
  // PT: Deliberadamente NUNCA chama load(), update() ou render() -- capture_frame() ainda
  //     deve funcionar, lendo o que o próprio host pôs no FBO 0.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(200.f / 255.f, 60.f / 255.f, 10.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  const glintfx::UiLayer::CapturedFrame frame = ui.capture_frame();
  if (!frame.ok) {
    std::puts("FAIL: part C capture_frame() ok == false");
    return false;
  }
  if (frame.width != 120 || frame.height != 80) {
    std::fprintf(stderr, "FAIL: part C size %dx%d, expected 120x80\n", frame.width, frame.height);
    return false;
  }
  unsigned char px[4];
  sample(frame, 60, 40, px);
  if (!looks_like(px[0], px[1], px[2], 200, 60, 10)) {
    std::fprintf(stderr, "FAIL: part C sample (%d,%d,%d) not host clear colour\n",
                 px[0], px[1], px[2]);
    return false;
  }
  std::puts("ui_layer_capture_frame_smoke: part C (before load) PASS");
  return true;
}

// EN: Part G (zero-width viewport, the DEGENERATE-SIZE guard -- team-lead follow-up,
//     `w21_framegrab_embed` SIGSEGV thread, 2026-07-30) -- exercises Engine::capture_frame's
//     own `w<=0||h<=0` defensive guard FOR REAL, closing a gap the App-side reviewer flagged
//     as untestable under Xvfb (no window manager to minimize a real window to 0x0, so
//     App::capture_frame's own w<=0||h<=0 branch was never live-fired). `set_viewport()`
//     itself CANNOT be used to inject this: its own guard (ui_layer.hpp) rejects
//     `w<=0||h<=0` and keeps the PREVIOUS viewport -- confirmed by reading the guard, not
//     assumed. The one legitimate, non-UB way in: `UiLayerConfig` is NOT validated by the
//     constructor before `Engine::attach()` -- `UiLayerConfig{.logical_width = 0, ...}`
//     reaches `Engine::attach()`/`Bootstrap::init()`/`Rml::CreateContext("main", {0, h})`
//     unchecked. MEASURED (not assumed) via a throwaway probe run before writing this
//     assertion: `ui.ok() == true` with `logical_width=0` -- RmlUi accepts a
//     zero-width context at creation time without crashing, so `capture_frame()`'s own guard
//     is the ONLY thing standing between this and a `glReadPixels(..., 0, h, ...)` call.
// PT: Parte G (viewport de largura zero, o guard de TAMANHO DEGENERADO -- desdobramento do
//     team-lead, thread do SIGSEGV do `w21_framegrab_embed`, 2026-07-30) -- exercita de
//     verdade o próprio guard defensivo `w<=0||h<=0` de Engine::capture_frame, fechando uma
//     lacuna que o reviewer do lado App sinalizou como intestável sob Xvfb (sem window
//     manager pra minimizar uma janela real a 0x0, então o próprio ramo w<=0||h<=0 de
//     App::capture_frame nunca disparou ao vivo). O próprio `set_viewport()` NÃO PODE ser
//     usado pra injetar isto: o próprio guard dele (ui_layer.hpp) rejeita `w<=0||h<=0` e
//     mantém o viewport ANTERIOR -- confirmado lendo o guard, não presumido. O único jeito
//     legítimo, não-UB, de entrar: `UiLayerConfig` NÃO é validado pelo construtor antes de
//     `Engine::attach()` -- `UiLayerConfig{.logical_width = 0, ...}` alcança
//     `Engine::attach()`/`Bootstrap::init()`/`Rml::CreateContext("main", {0, h})` sem
//     checagem. MEDIDO (não presumido) via uma sonda descartável rodada antes de escrever
//     esta asserção: `ui.ok() == true` com `logical_width=0` -- o RmlUi aceita um contexto
//     de largura zero na criação sem crashar, então o próprio guard do `capture_frame()` é a
//     ÚNICA coisa entre isto e uma chamada `glReadPixels(..., 0, h, ...)`.
bool part_g_zero_viewport() {
  glintfx::WindowGlfw host;
  if (!host.create("capture_zero_viewport_host", 100, 100)) {
    std::puts("FAIL: part G host create");
    return false;
  }
  glintfx::UiLayer ui({.logical_width = 0, .logical_height = 100, .load_gl = true});
  if (!ui.ok()) {
    // EN: If a future RmlUi/engine.attach() version starts rejecting a zero-width context,
    //     this branch documents that -- capture_frame()'s own guard becomes unreachable this
    //     way, but the pre-existing !ok() guard already covers the resulting CapturedFrame{}.
    // PT: Se uma versão futura do RmlUi/engine.attach() passar a rejeitar um contexto de
    //     largura zero, este ramo documenta isso -- o próprio guard do capture_frame() fica
    //     inalcançável por este caminho, mas o guard !ok() pré-existente já cobre o
    //     CapturedFrame{} resultante.
    std::puts(
        "ui_layer_capture_frame_smoke: part G (zero viewport) PASS -- attach() itself "
        "rejected width=0, !ok() guard covers it");
    return true;
  }
  ui.update();
  ui.render();
  const glintfx::UiLayer::CapturedFrame frame = ui.capture_frame();
  if (frame.ok) {
    std::puts("FAIL: part G capture_frame() ok == true with a 0-width viewport");
    return false;
  }
  if (frame.width != 0 || frame.height != 0 || frame.byte_count != 0 || frame.pixels) {
    std::puts("FAIL: part G CapturedFrame not fully default on the w<=0||h<=0 guard");
    return false;
  }
  std::puts("ui_layer_capture_frame_smoke: part G (zero viewport) PASS");
  return true;
}

// EN: NO "moved-from" / "no current GL context" sub-tests here -- DELIBERATELY, after
//     measuring both, not by omission. See this file's own header comment ("Edge cases
//     considered and rejected") for the full rationale and the crash this decision is based
//     on.
// PT: SEM sub-testes de "movido-de" / "sem contexto GL corrente" aqui -- DELIBERADAMENTE,
//     após medir os dois, não por omissão. Ver o próprio comentário de cabeçalho deste
//     arquivo ("Casos-limite considerados e rejeitados") pro racional completo e o crash em
//     que esta decisão se baseia.

bool part_e_letterbox_region() {
  glintfx::WindowGlfw host;
  if (!host.create("capture_letterbox_host", 400, 300)) {
    std::puts("FAIL: part E host create");
    return false;
  }
  glintfx::UiLayer ui({.logical_width = 200, .logical_height = 150, .load_gl = true});
  if (!ui.ok()) {
    std::puts("FAIL: part E ui attach");
    return false;
  }
  if (!ui.load("tests/min_partial.rml")) {
    std::puts("FAIL: part E load");
    return false;
  }
  // EN: Sub-region (x=50,y=30,w=200,h=150) inside a 400x300 window (target_h=300).
  // PT: Sub-região (x=50,y=30,w=200,h=150) dentro de uma janela 400x300 (target_h=300).
  ui.set_viewport(50, 30, 200, 150, 300);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(5.f / 255.f, 5.f / 255.f, 5.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT); // EN: clears the WHOLE window. PT: limpa a JANELA INTEIRA.

  ui.update();
  ui.render();

  const glintfx::UiLayer::CapturedFrame frame = ui.capture_frame();
  if (!frame.ok) {
    std::puts("FAIL: part E capture_frame() ok == false");
    return false;
  }
  if (frame.width != 200 || frame.height != 150) {
    std::fprintf(stderr,
                 "FAIL: part E size %dx%d, expected 200x150 (region ceiling not respected)\n",
                 frame.width, frame.height);
    return false;
  }
  // EN: #box's own local centre (10,10)..(90,50) -> centre (50,30), in the SUB-VIEWPORT's own
  //     local coordinate space (the returned buffer's own (0,0) is the sub-viewport's origin).
  // PT: O próprio centro local do #box (10,10)..(90,50) -> centro (50,30), no próprio espaço
  //     de coordenada local da SUB-VIEWPORT (o (0,0) do próprio buffer devolvido é a origem
  //     da sub-viewport).
  unsigned char px[4];
  sample(frame, 50, 30, px);
  if (!looks_like(px[0], px[1], px[2], 255, 255, 255)) {
    std::fprintf(stderr,
                 "FAIL: part E #box local-centre sample (%d,%d,%d) not white -- offset "
                 "conversion likely reading the wrong window rectangle\n",
                 px[0], px[1], px[2]);
    return false;
  }
  // EN: (150, 100) is clear of #box's own (10,10)..(90,50) footprint on BOTH axes (same
  //     margin Part A/B's own background sample uses against the SAME fixture) -- (10, 10)
  //     would have sampled the box's own top-left CORNER, not the background (an off-by-
  //     geometry mistake caught by running this test, not by inspection).
  // PT: (150, 100) está livre da própria pegada do #box (10,10)..(90,50) nos DOIS eixos
  //     (mesma margem que a própria amostra de fundo de Parte A/B usa contra a MESMA
  //     fixture) -- (10, 10) teria amostrado o próprio CANTO superior-esquerdo do box, não o
  //     fundo (um erro de geometria pego rodando este teste, não por inspeção).
  sample(frame, 150, 100, px);
  if (!looks_like(px[0], px[1], px[2], 5, 5, 5)) {
    std::fprintf(stderr, "FAIL: part E background sample (%d,%d,%d) not anchor\n",
                 px[0], px[1], px[2]);
    return false;
  }
  std::puts("ui_layer_capture_frame_smoke: part E (letterbox region) PASS");
  return true;
}

bool part_f_odd_width() {
  const int W = 301, H = 200; // EN: ODD width. PT: largura ÍMPAR.
  glintfx::WindowGlfw host;
  if (!host.create("capture_odd_width_host", W, H)) {
    std::puts("FAIL: part F host create");
    return false;
  }
  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("FAIL: part F ui attach");
    return false;
  }
  // EN: No document loaded -- pure readback correctness under an odd width, independent of
  //     RmlUi content.
  // PT: Nenhum documento carregado -- correção de readback pura sob largura ímpar,
  //     independente de conteúdo do RmlUi.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  const int ER = 100, EG = 150, EB = 200;
  glClearColor(ER / 255.f, EG / 255.f, EB / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  ui.update();
  ui.render();

  const glintfx::UiLayer::CapturedFrame frame = ui.capture_frame();
  if (!frame.ok) {
    std::puts("FAIL: part F capture_frame() ok == false");
    return false;
  }
  if (frame.width != W || frame.height != H) {
    std::fprintf(stderr, "FAIL: part F size %dx%d, expected %dx%d\n", frame.width, frame.height, W, H);
    return false;
  }

  // EN: Corners + several interior/edge points across MULTIPLE rows -- a PACK_ALIGNMENT bug
  //     corrupts rows AFTER the first, so row 0 alone would not catch it.
  // PT: Cantos + vários pontos internos/de borda através de MÚLTIPLAS linhas -- um bug de
  //     PACK_ALIGNMENT corrompe linhas APÓS a primeira, então só a linha 0 não pegaria.
  const int xs[] = {0, 1, W / 2, W - 2, W - 1};
  const int ys[] = {0, 1, H / 4, H / 2, (3 * H) / 4, H - 2, H - 1};
  bool pass = true;
  for (int y : ys) {
    for (int x : xs) {
      unsigned char px[4];
      sample(frame, x, y, px);
      if (!looks_like(px[0], px[1], px[2], ER, EG, EB, /*tol=*/4)) {
        std::fprintf(stderr,
                     "FAIL: part F pixel (%d,%d) = (%d,%d,%d), expected (%d,%d,%d) -- "
                     "row/column corruption (odd-width PACK_ALIGNMENT regression)\n",
                     x, y, px[0], px[1], px[2], ER, EG, EB);
        pass = false;
      }
    }
  }
  if (!pass) return false;
  std::puts("ui_layer_capture_frame_smoke: part F (odd width) PASS");
  return true;
}

} // namespace

int main() {
  bool pass = true;

  {
    glintfx::WindowGlfw host;
    if (!host.create("capture_ab_host", 300, 200)) {
      std::puts("FAIL: parts A/B host create");
      return 1;
    }
    glintfx::UiLayer ui({.logical_width = 300, .logical_height = 200, .load_gl = true});
    if (!ui.ok()) {
      std::puts("FAIL: parts A/B ui attach");
      return 2;
    }
    if (!ui.load("tests/min_partial.rml")) {
      std::puts("FAIL: parts A/B load");
      return 3;
    }
    if (!part_a_and_b(host, ui)) pass = false;
  }

  if (!part_c_before_load()) pass = false;
  if (!part_g_zero_viewport()) pass = false;
  if (!part_e_letterbox_region()) pass = false;
  if (!part_f_odd_width()) pass = false;

  if (!pass) return 10;
  std::puts("ui_layer_capture_frame_smoke: PASS");
  return 0;
}
