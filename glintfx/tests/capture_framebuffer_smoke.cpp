// SPDX-License-Identifier: MPL-2.0
// EN: capture_framebuffer_smoke (CAPTURE-FREE, W22 S8, 2026-07-30) -- verifies
//     glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp), the
//     instance-free sibling of App::capture_frame()/UiLayer::capture_frame(). Unlike either
//     sibling's own smoke test, this one needs NO UiLayer/RmlUi document at all -- a plain
//     WindowGlfw host (this file's OWN GL painting via glScissor/glClearColor) is the entire
//     fixture, proving the function's central design promise: reading pixels back does not
//     require knowing RmlUi exists.
//
//     Part A -- COORDINATE CONTRACT PROOF (the ONE thing this free function does differently
//     from its two instance-method siblings, per frame_capture.hpp's own top comment): paints
//     a RED strip at GL-NATIVE y in [0, STRIP_H) -- the BOTTOM of the window in GL terms --
//     leaving the rest of the window at a distinct background colour, then captures the WHOLE
//     window with capture_framebuffer(0, 0, W, H). Because CapturedFramebuffer::pixels row 0
//     is documented TOP-of-image (same convention as its siblings), the flip must place the
//     GL-bottom red strip at the LAST returned rows, and the background at the FIRST returned
//     rows -- proving BOTH the top-down flip AND that (gl_x, gl_y) = (0, 0) means GL-native
//     bottom-left, NOT window-space top-left (had this function silently done a window-space
//     conversion nobody asked for, this specific assertion would flip and this test would
//     fail).
//
//     Part B -- ARBITRARY SUB-REGION PROOF (the NEW capability neither instance-method sibling
//     exposes to its own caller: App always captures the whole window, UiLayer always captures
//     its own precomputed letterbox rect): paints a second, small, off-origin GREEN square at
//     a GL-native rect NOT anchored at (0,0), then calls capture_framebuffer() with EXACTLY
//     that rect's own (gl_x, gl_y, w, h) -- every returned pixel must be green, proving the
//     caller-supplied region is honoured verbatim, not silently reinterpreted.
//
//     Part C -- WIDTH-NOT-A-MULTIPLE-OF-4 PACK_ALIGNMENT REGRESSION (same class, same width
//     value, as ui_layer_capture_frame_smoke.cpp's own Part F -- gcd(3,4)=1, so this is NOT
//     "odd width", an EVEN width such as 2/6/198 corrupts identically; W=301 is one member of
//     the class, kept identical to the precedent test's own choice on purpose so a future
//     reader recognises the same regression being re-proven for a second entry point sharing
//     the SAME duplicated readback body, frame_capture.cpp's own top comment). Samples corners
//     plus several interior/edge points across MULTIPLE rows -- a PACK_ALIGNMENT bug corrupts
//     rows AFTER the first, so row 0 alone would not catch it.
//
//     Part D -- FAIL-HIGH GUARDS: w<=0, h<=0, and a negative w all degrade to a clean
//     CapturedFramebuffer{} (ok==false, width==height==0, pixels==nullptr, byte_count==0),
//     never a crash, matching Engine::capture_frame()'s own guard shape.
// PT: capture_framebuffer_smoke (CAPTURE-FREE, W22 S8, 2026-07-30) -- verifica
//     glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp), a irmã sem
//     instância de App::capture_frame()/UiLayer::capture_frame(). Diferente do próprio smoke
//     test de qualquer uma das duas irmãs, este não precisa de UiLayer/documento RmlUi nenhum
//     -- um host WindowGlfw simples (a própria pintura GL deste arquivo via
//     glScissor/glClearColor) é a fixture inteira, provando a promessa de desenho central da
//     função: ler pixels de volta não exige saber que a RmlUi existe.
//
//     Parte A -- PROVA DO CONTRATO DE COORDENADAS (a ÚNICA coisa que esta função livre faz
//     diferente das duas irmãs de método de instância, pelo próprio comentário de topo de
//     frame_capture.hpp): pinta uma faixa VERMELHA em y NATIVO-GL em [0, STRIP_H) -- o FUNDO da
//     janela em termos GL -- deixando o resto da janela numa cor de fundo distinta, depois
//     captura a janela INTEIRA com capture_framebuffer(0, 0, W, H). Como
//     CapturedFramebuffer::pixels linha 0 é documentada como o TOPO da imagem (mesma convenção
//     das irmãs dela), a inversão precisa colocar a faixa vermelha do fundo-GL nas ÚLTIMAS
//     linhas devolvidas, e o fundo nas PRIMEIRAS linhas devolvidas -- provando TANTO a
//     inversão top-down QUANTO que (gl_x, gl_y) = (0, 0) significa inferior-esquerda
//     nativo-GL, NÃO superior-esquerda em espaço-janela (se esta função tivesse feito
//     silenciosamente uma conversão de espaço-janela que ninguém pediu, esta asserção
//     específica se inverteria e este teste falharia).
//
//     Parte B -- PROVA DE SUB-REGIÃO ARBITRÁRIA (a capacidade NOVA que nenhuma das duas irmãs
//     de método de instância expõe ao próprio chamador: o App sempre captura a janela inteira,
//     o UiLayer sempre captura o próprio retângulo de letterbox pré-computado): pinta um
//     segundo quadrado pequeno VERDE, fora da origem, num retângulo nativo-GL NÃO ancorado em
//     (0,0), depois chama capture_framebuffer() com EXATAMENTE o próprio (gl_x, gl_y, w, h)
//     daquele retângulo -- todo pixel devolvido precisa ser verde, provando que a região
//     fornecida pelo chamador é honrada ao pé da letra, não reinterpretada silenciosamente.
//
//     Parte C -- REGRESSÃO DE GL_PACK_ALIGNMENT COM LARGURA NÃO MÚLTIPLA DE 4 (mesma classe,
//     mesmo valor de largura, da própria Parte F de ui_layer_capture_frame_smoke.cpp --
//     mdc(3,4)=1, então isto NÃO é "largura ímpar", uma largura PAR como 2/6/198 corrompe
//     idêntico; W=301 é um membro da classe, mantido idêntico à própria escolha do teste
//     precedente de propósito pra um leitor futuro reconhecer a mesma regressão sendo
//     reprovada pra um segundo ponto de entrada compartilhando o MESMO corpo de readback
//     duplicado, o próprio comentário de topo de frame_capture.cpp). Amostra cantos mais
//     vários pontos internos/de borda através de MÚLTIPLAS linhas -- um bug de PACK_ALIGNMENT
//     corrompe linhas APÓS a primeira, então só a linha 0 não pegaria.
//
//     Parte D -- GUARDAS FAIL-HIGH: w<=0, h<=0, e um w negativo todos degradam pra um
//     CapturedFramebuffer{} limpo (ok==false, width==height==0, pixels==nullptr,
//     byte_count==0), nunca um crash, batendo com a própria forma de guarda do
//     Engine::capture_frame().
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h (glClear/glScissor/etc used directly below). PT: inclui gl_loader.h (glClear/glScissor/etc usados abaixo).
#include <cstdio>

namespace {

bool looks_like(unsigned char r, unsigned char g, unsigned char b,
                int er, int eg, int eb, int tol = 6) {
  auto close = [tol](int a, int b2) { return (a - b2 >= -tol) && (a - b2 <= tol); };
  return close(r, er) && close(g, eg) && close(b, eb);
}

void sample(const glintfx::CapturedFramebuffer& frame, int x, int y, unsigned char out[4]) {
  const std::size_t idx =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width) +
       static_cast<std::size_t>(x)) *
      4;
  out[0] = frame.pixels[idx + 0];
  out[1] = frame.pixels[idx + 1];
  out[2] = frame.pixels[idx + 2];
  out[3] = frame.pixels[idx + 3];
}

bool part_a_and_b() {
  const int W = 64, H = 48, STRIP_H = 12;
  glintfx::WindowGlfw host;
  if (!host.create("capture_framebuffer_ab_host", W, H)) {
    std::puts("FAIL: part A/B host create");
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_SCISSOR_TEST);
  // EN: Background: distinct dark blue-grey, nowhere near red or green below.
  // PT: Fundo: azul-acinzentado escuro distinto, longe do vermelho ou verde abaixo.
  glClearColor(20.f / 255.f, 30.f / 255.f, 60.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // EN: Part A fixture -- RED strip at GL-native y in [0, STRIP_H), spanning the full width.
  // PT: Fixture da parte A -- faixa VERMELHA em y nativo-GL em [0, STRIP_H), largura inteira.
  glEnable(GL_SCISSOR_TEST);
  glScissor(0, 0, W, STRIP_H);
  glClearColor(220.f / 255.f, 20.f / 255.f, 20.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // EN: Part B fixture -- small GREEN square, off-origin, clear of the red strip
  //     (y in [20,28), entirely above STRIP_H=12).
  // PT: Fixture da parte B -- quadrado VERDE pequeno, fora da origem, livre da faixa vermelha
  //     (y em [20,28), inteiramente acima de STRIP_H=12).
  const int gx = 20, gy = 20, gw = 8, gh = 8;
  glScissor(gx, gy, gw, gh);
  glClearColor(20.f / 255.f, 200.f / 255.f, 20.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);

  // EN: Part A -- whole-window capture.
  // PT: Parte A -- captura da janela inteira.
  const glintfx::CapturedFramebuffer whole = glintfx::capture_framebuffer(0, 0, W, H);
  if (!whole.ok) {
    std::puts("FAIL: part A capture_framebuffer() ok == false");
    return false;
  }
  if (whole.width != W || whole.height != H) {
    std::fprintf(stderr, "FAIL: part A size %dx%d, expected %dx%d\n",
                 whole.width, whole.height, W, H);
    return false;
  }
  const std::size_t expected_bytes = static_cast<std::size_t>(W) * static_cast<std::size_t>(H) * 4;
  if (whole.byte_count != expected_bytes) {
    std::fprintf(stderr, "FAIL: part A byte_count=%zu expected %zu\n", whole.byte_count, expected_bytes);
    return false;
  }
  if (!whole.pixels) {
    std::puts("FAIL: part A pixels null despite ok");
    return false;
  }

  // EN: TOP row of the returned (top-down) buffer must be the BACKGROUND colour -- it maps to
  //     GL row H-1, far above the [0, STRIP_H) red strip.
  // PT: Linha do TOPO do buffer devolvido (top-down) precisa ser a cor de FUNDO -- mapeia pra
  //     linha GL H-1, bem acima da faixa vermelha [0, STRIP_H).
  unsigned char top_px[4];
  sample(whole, W / 2, 0, top_px);
  if (!looks_like(top_px[0], top_px[1], top_px[2], 20, 30, 60)) {
    std::fprintf(stderr,
                 "FAIL: part A top row (%d,%d,%d) is not background -- coordinate contract "
                 "broken (gl_y=0 did not read the BOTTOM of the GL-native window)\n",
                 top_px[0], top_px[1], top_px[2]);
    return false;
  }

  // EN: LAST row of the returned buffer must be the RED strip -- it maps to GL row 0.
  // PT: ÚLTIMA linha do buffer devolvido precisa ser a faixa VERMELHA -- mapeia pra linha GL 0.
  unsigned char bottom_px[4];
  sample(whole, W / 2, H - 1, bottom_px);
  if (!looks_like(bottom_px[0], bottom_px[1], bottom_px[2], 220, 20, 20)) {
    std::fprintf(stderr,
                 "FAIL: part A last row (%d,%d,%d) is not the red strip -- top-down flip is "
                 "broken\n",
                 bottom_px[0], bottom_px[1], bottom_px[2]);
    return false;
  }
  if (bottom_px[3] != 255) {
    std::puts("FAIL: part A synthetic alpha != 255");
    return false;
  }

  // EN: Part B -- sub-region capture, exactly the green square's own GL-native rect.
  // PT: Parte B -- captura de sub-região, exatamente o próprio retângulo nativo-GL do quadrado
  //     verde.
  const glintfx::CapturedFramebuffer region = glintfx::capture_framebuffer(gx, gy, gw, gh);
  if (!region.ok) {
    std::puts("FAIL: part B capture_framebuffer() ok == false");
    return false;
  }
  if (region.width != gw || region.height != gh) {
    std::fprintf(stderr, "FAIL: part B size %dx%d, expected %dx%d\n",
                 region.width, region.height, gw, gh);
    return false;
  }
  bool all_green = true;
  for (int y = 0; y < gh; ++y) {
    for (int x = 0; x < gw; ++x) {
      unsigned char px[4];
      sample(region, x, y, px);
      if (!looks_like(px[0], px[1], px[2], 20, 200, 20)) {
        std::fprintf(stderr,
                     "FAIL: part B pixel (%d,%d) = (%d,%d,%d), expected green -- sub-region "
                     "not honoured verbatim\n",
                     x, y, px[0], px[1], px[2]);
        all_green = false;
      }
    }
  }
  if (!all_green) return false;

  std::puts("capture_framebuffer_smoke: parts A/B PASS");
  return true;
}

bool part_c_width_not_multiple_of_4() {
  const int W = 301, H = 64;
  glintfx::WindowGlfw host;
  if (!host.create("capture_framebuffer_unaligned_width_host", W, H)) {
    std::puts("FAIL: part C host create");
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_SCISSOR_TEST);
  const int ER = 100, EG = 150, EB = 200;
  glClearColor(ER / 255.f, EG / 255.f, EB / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  const glintfx::CapturedFramebuffer frame = glintfx::capture_framebuffer(0, 0, W, H);
  if (!frame.ok) {
    std::puts("FAIL: part C capture_framebuffer() ok == false");
    return false;
  }
  if (frame.width != W || frame.height != H) {
    std::fprintf(stderr, "FAIL: part C size %dx%d, expected %dx%d\n", frame.width, frame.height, W, H);
    return false;
  }

  const int xs[] = {0, 1, W / 2, W - 2, W - 1};
  const int ys[] = {0, 1, H / 4, H / 2, (3 * H) / 4, H - 2, H - 1};
  bool pass = true;
  for (int y : ys) {
    for (int x : xs) {
      unsigned char px[4];
      sample(frame, x, y, px);
      if (!looks_like(px[0], px[1], px[2], ER, EG, EB, /*tol=*/4)) {
        std::fprintf(stderr,
                     "FAIL: part C pixel (%d,%d) = (%d,%d,%d), expected (%d,%d,%d) -- "
                     "row/column corruption (width-not-multiple-of-4 PACK_ALIGNMENT regression)\n",
                     x, y, px[0], px[1], px[2], ER, EG, EB);
        pass = false;
      }
    }
  }
  if (!pass) return false;
  std::puts("capture_framebuffer_smoke: part C (width not multiple of 4) PASS");
  return true;
}

bool part_d_fail_high_guards() {
  glintfx::WindowGlfw host;
  if (!host.create("capture_framebuffer_guards_host", 32, 32)) {
    std::puts("FAIL: part D host create");
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  auto expect_degenerate = [](const glintfx::CapturedFramebuffer& f, const char* label) {
    if (f.ok || f.width != 0 || f.height != 0 || f.pixels != nullptr || f.byte_count != 0) {
      std::fprintf(stderr, "FAIL: part D %s did not degrade to CapturedFramebuffer{}\n", label);
      return false;
    }
    return true;
  };

  bool pass = true;
  pass &= expect_degenerate(glintfx::capture_framebuffer(0, 0, 0, 16), "w=0");
  pass &= expect_degenerate(glintfx::capture_framebuffer(0, 0, 16, 0), "h=0");
  pass &= expect_degenerate(glintfx::capture_framebuffer(0, 0, -5, 16), "w=-5");
  pass &= expect_degenerate(glintfx::capture_framebuffer(0, 0, 16, -5), "h=-5");
  if (!pass) return false;

  // EN: A real capture right after the guards must STILL succeed -- the guards must not have
  //     touched GL state on the invalid paths.
  // PT: Uma captura real logo após as guardas precisa AINDA ter sucesso -- as guardas não
  //     podem ter tocado estado GL nos caminhos inválidos.
  const glintfx::CapturedFramebuffer real = glintfx::capture_framebuffer(0, 0, 16, 16);
  if (!real.ok) {
    std::puts("FAIL: part D real capture after guards returned ok == false");
    return false;
  }

  std::puts("capture_framebuffer_smoke: part D (fail-high guards) PASS");
  return true;
}

} // namespace

int main() {
  bool pass = true;
  if (!part_a_and_b()) pass = false;
  if (!part_c_width_not_multiple_of_4()) pass = false;
  if (!part_d_fail_high_guards()) pass = false;

  if (pass) {
    std::puts("capture_framebuffer_smoke: PASS");
    return 0;
  }
  std::puts("capture_framebuffer_smoke: FAILED");
  return 1;
}
