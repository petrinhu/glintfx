// SPDX-License-Identifier: MPL-2.0
// EN: ripple_stress_sanity -- INDEPENDENT adversarial gate for commit 43cdcfa ("fix(glintfx):
//     ripple cold-start -- captura on-demand dentro de RenderShader, não gate por contador").
//     Written by the reviewer (NOT the qa who wrote ripple_sanity.cpp, NOT the implementer of
//     43cdcfa) as the security-review counter-proof this fix's own risk demands: the real GL
//     backdrop capture (EnsureBackdropCaptured, render_gl3.cpp) now runs from INSIDE
//     Rml::Context::Render()'s tree walk (from RenderShader's "glintfx-ripple" branch), i.e.
//     mid-frame, DURING RmlUi's own rendering of the document -- not before it (as the old,
//     buggy `CaptureBackdrop` design did, from begin_frame_compose). ripple_sanity.cpp already
//     proves the capture itself is pixel-correct and fires on the cold-start frame; it does NOT
//     prove the capture's own GL calls (glBindFramebuffer(GL_READ_FRAMEBUFFER,0)+
//     glReadBuffer(GL_BACK)+glGenTextures/glTexImage2D/glCopyTexSubImage2D+glActiveTexture(0)+
//     glBindTexture) leave the GL context in a state every OTHER element painted LATER in the
//     SAME Context::Render() tree walk can still rely on.
//
//     METHODOLOGY -- differential (self-referential), not hand-derived-math: the SAME document
//     is rendered TWICE, back to back, on the SAME UiLayer/host window, with #ripple_el's own
//     "ripple()" decorator toggled OFF (run A, EnsureBackdropCaptured() never even reachable --
//     confirmed via the SAME glCopyTexSubImage2D spy idiom ripple_sanity.cpp uses) then ON (run
//     B, EnsureBackdropCaptured() fires mid-tree-walk for real). FOUR "after" elements --
//     #rect_after (native RenderGeometry "Color" program, no texture/shader at all),
//     #image_after (native RenderGeometry "Texture" program, decorator:image, a REAL GL texture
//     on unit 0), #gradient_after (RenderShader's delegate-to-base path -- RmlUi's OWN
//     "linear-gradient" shader, code glintfx does not own or touch), #tint_after (glintfx's OWN
//     "is_tint" RenderShader branch, decorator:image-tint -- the branch structurally ADJACENT to
//     the ripple branch under review, sharing its glActiveTexture(GL_TEXTURE0) idiom) -- each sit
//     LATER than #ripple_el in document order (painted AFTER it in the SAME tree walk, see
//     ripple_stress_scene.rcss's own top comment) and are diffed PIXEL-FOR-PIXEL, bit-exact,
//     between run A and run B. If EnsureBackdropCaptured()'s mid-frame GL calls corrupted ANY
//     state a later element's own draw call implicitly relies on (a program/VAO/texture-unit
//     binding NOT explicitly reset before use), the corresponding element's pixels in run B would
//     differ from run A -- this is the ONLY way to falsify "safe mid-frame capture" empirically,
//     since a purely visual/rendered-output check of #ripple_el itself (already done by
//     ripple_sanity.cpp) says nothing about elements painted after it.
//
//     A SECOND, independent check (host-state sentinel) additionally proves the mid-frame capture
//     does not leak past UiLayer::render() into the HOST's own GL context: before calling
//     render(), this file binds a host-owned sentinel texture on a NON-zero texture unit
//     (GL_TEXTURE5, deliberately far from the GL_TEXTURE0 unit EnsureBackdropCaptured/the ripple
//     shader/the tint shader all use) and checks GL_ACTIVE_TEXTURE + that unit's
//     GL_TEXTURE_BINDING_2D are back to the sentinel afterwards -- GlStateGuard (gl_state.hpp)
//     only saves/restores the SINGLE unit that was active at its own ctor, so this specifically
//     exercises that this is indeed the active unit at ctor-time and correctly round-trips.
// PT: ripple_stress_sanity -- GATE ADVERSARIAL INDEPENDENTE para o commit 43cdcfa. Ver os
//     parágrafos EN acima (espelhados aqui em conceito): metodologia DIFERENCIAL -- o MESMO
//     documento é renderizado DUAS vezes, com o decorator "ripple()" de #ripple_el desligado
//     (rodada A) e ligado (rodada B), e os pixels de QUATRO elementos "after" (cada um exercitando
//     um caminho de draw DIFERENTE) são comparados BIT-EXATOS entre as duas rodadas -- qualquer
//     divergência prova que a captura no meio do frame corrompeu estado que um draw POSTERIOR
//     dependia implicitamente. Um segundo check (sentinela de estado do host) prova que a captura
//     não vaza pro contexto GL do PRÓPRIO host depois que UiLayer::render() retorna.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"  // EN: includes gl_loader.h (GL already loaded by WindowGlfw::create).
                          // PT: inclui gl_loader.h (GL já carregado por WindowGlfw::create).
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int W = 900, H = 800;

// ===============================================================================================
// EN: Same glCopyTexSubImage2D spy idiom as ripple_sanity.cpp -- see that file's own top comment
//     for the full linkage-safety reasoning (identical here: this target links `glintfx glloader`
//     so there is exactly ONE glCopyTexSubImage2D global in the final binary).
// PT: Mesmo idioma de espião de ripple_sanity.cpp -- ver o próprio comentário do topo daquele
//     arquivo pra racional completa de segurança de linkagem.
// ===============================================================================================
int g_capture_calls = 0;
PFNGLCOPYTEXSUBIMAGE2DPROC g_real_copy_tex_sub_image_2d = nullptr;

void APIENTRY SpyCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLint x, GLint y, GLsizei width, GLsizei height) {
  ++g_capture_calls;
  if (g_real_copy_tex_sub_image_2d)
    g_real_copy_tex_sub_image_2d(target, level, xoffset, yoffset, x, y, width, height);
}

std::vector<unsigned char> render_once(glintfx::UiLayer& ui) {
  ui.update();
  ui.render();
  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

// EN: document/content-space (top-left origin, y-down), same "row = h-1-content_y" convention as
//     ripple_sanity.cpp's own pixel_at().
// PT: espaço-conteúdo (origem topo-esquerda, y pra baixo), mesma convenção de ripple_sanity.cpp.
const unsigned char* pixel_at(const std::vector<unsigned char>& px, int content_x, int content_y) {
  const int row = H - 1 - content_y;
  return &px[(static_cast<size_t>(row) * W + content_x) * 3];
}

struct Box { const char* name; int cx, cy; };  // cx/cy = a sample point WELL inside the box (not
                                                // on an anti-aliased edge), content-space.
// EN: One interior sample point per "after" box -- see ripple_stress_scene.rcss for box geometry
//     (all boxes: top=620, height 60 or 140). Centres chosen a few px in from every edge.
// PT: Um ponto de amostra interior por caixa "after" -- ver ripple_stress_scene.rcss pra
//     geometria das caixas. Centros escolhidos alguns px pra dentro de toda borda.
constexpr Box kAfterBoxes[] = {
    {"rect_after",     110, 690},  // solid #e05050, native Color program.
    {"image_after",    330, 650},  // decorator:image, native Texture program.
    {"gradient_after", 550, 690},  // decorator:linear-gradient, delegated base RenderShader.
    {"tint_after",     770, 650},  // decorator:image-tint, glintfx's own tint RenderShader.
};

}  // namespace

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("ripple_stress_sanity_host", W, H)) {
    std::puts("ripple_stress_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("ripple_stress_sanity FAIL: ui attach failed");
    return 2;
  }

  // EN: Install the spy AFTER every glx_gl_load() call this process makes -- see
  //     ripple_sanity.cpp's own comment on this exact ordering requirement (host.create() above
  //     AND UiLayer's own ctor both call glx_gl_load(), which unconditionally re-resolves EVERY
  //     GL function-pointer global on every call).
  // PT: Instala o espião DEPOIS de toda chamada a glx_gl_load() -- ver o comentário de
  //     ripple_sanity.cpp sobre este requisito exato de ordenação.
  g_real_copy_tex_sub_image_2d = glCopyTexSubImage2D;
  glCopyTexSubImage2D = &SpyCopyTexSubImage2D;

  // EN: Arbitrary solid backdrop -- content is irrelevant here (ripple's OWN visual correctness
  //     is ripple_sanity.cpp's job, not this file's); only needs w>0,h>0 for
  //     EnsureBackdropCaptured() to actually perform its GL calls in run B, below.
  // PT: Backdrop sólido arbitrário -- o conteúdo é irrelevante aqui; só precisa de w>0,h>0 pra
  //     EnsureBackdropCaptured() de fato realizar suas chamadas GL na rodada B, abaixo.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, W, H);
  glClearColor(0.1f, 0.3f, 0.4f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  const bool ok_load = ui.load("ripple_stress_scene.rml");
  if (!ok_load) {
    std::puts("ripple_stress_sanity FAIL: load failed");
    return 3;
  }

  int failures = 0;

  // ===========================================================================================
  // Run A -- #ripple_el INACTIVE (no "active" class yet -- decorator never attaches).
  //   EnsureBackdropCaptured() must be structurally unreachable (spy count stays 0), mirroring
  //   ripple_sanity.cpp's own Proof 2a for the same document-structural reason.
  // ===========================================================================================
  g_capture_calls = 0;
  const auto px_a = render_once(ui);
  std::printf("ripple_stress_sanity [run A, ripple inactive]: capture calls = %d\n", g_capture_calls);
  if (g_capture_calls != 0) {
    std::puts("ripple_stress_sanity FAIL [A0]: glCopyTexSubImage2D fired with the ripple decorator "
               "INACTIVE -- EnsureBackdropCaptured is reachable when it should not be (scene setup "
               "bug, not what this file means to test) -- aborting, results below would be invalid");
    return 4;
  }

  // Activate #ripple_el's ripple() decorator for run B.
  if (!ui.add_class("ripple_el", "active")) {
    std::puts("ripple_stress_sanity FAIL: add_class(ripple_el, active) rejected");
    return 5;
  }

  // ===========================================================================================
  // Run B -- #ripple_el ACTIVE. EnsureBackdropCaptured() must fire mid-Context::Render() at least
  //   once (spy count >= 1) -- this is the actual STRESS: the real GL capture (glBindFramebuffer/
  //   glReadBuffer/glGenTextures/glTexImage2D/glCopyTexSubImage2D/glActiveTexture/glBindTexture)
  //   now runs interleaved with the SAME Context::Render() tree walk that still has to paint the
  //   four #*_after elements below.
  // ===========================================================================================
  g_capture_calls = 0;
  const auto px_b = render_once(ui);
  std::printf("ripple_stress_sanity [run B, ripple active]: capture calls = %d\n", g_capture_calls);
  if (g_capture_calls < 1) {
    std::puts("ripple_stress_sanity FAIL [B0]: glCopyTexSubImage2D did NOT fire with the ripple "
               "decorator ACTIVE -- EnsureBackdropCaptured never ran, so this file's central "
               "stress (four post-ripple elements painting right after a mid-frame GL capture) "
               "never actually happened -- aborting, results below would be invalid");
    return 6;
  }

  // ===========================================================================================
  // THE central proof: every "after" element's pixels must be BIT-EXACT between run A (no
  // mid-frame capture ever happened) and run B (mid-frame capture happened, immediately before
  // these same elements painted). Any divergence is corruption caused by EnsureBackdropCaptured's
  // (or the ripple draw's own) GL calls leaking into a LATER element's implicit state assumptions.
  // ===========================================================================================
  for (const auto& box : kAfterBoxes) {
    const auto* a = pixel_at(px_a, box.cx, box.cy);
    const auto* b = pixel_at(px_b, box.cx, box.cy);
    std::printf("ripple_stress_sanity [%s @%d,%d]: run A=(%d,%d,%d) run B=(%d,%d,%d)\n",
        box.name, box.cx, box.cy, a[0], a[1], a[2], b[0], b[1], b[2]);
    if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
      std::printf("ripple_stress_sanity FAIL [STRESS/%s]: pixel differs between run A (no mid-frame "
                  "capture) and run B (mid-frame capture happened before this element painted) -- "
                  "EnsureBackdropCaptured's GL calls corrupted state this element's own draw path "
                  "implicitly relied on\n", box.name);
      ++failures;
    }
  }

  // ===========================================================================================
  // Host-state sentinel -- proves the corruption risk does not extend PAST UiLayer::render() into
  // the calling host's own GL context (complementary to GlStateGuard's own read-framebuffer proof
  // in ripple_sanity.cpp's Proof 4). Bind a host-owned 1x1 texture on GL_TEXTURE5 -- deliberately
  // far from GL_TEXTURE0, which EnsureBackdropCaptured/the ripple shader/the tint shader all use
  // internally. CRITICALLY, the READ-framebuffer sentinel below is bound to a REAL, NON-ZERO FBO
  // (not left at the default 0) -- a sentinel of "still 0 after render()" is a BLIND SPOT: since
  // EnsureBackdropCaptured's own capture ends with GL_READ_FRAMEBUFFER bound to 0 (that is what it
  // reads from), a host whose original read-fb binding WAS ALREADY 0 cannot distinguish "GlStateGuard
  // actually restored it" from "the capture's own leftover binding happens to coincide with 0" --
  // confirmed empirically while writing this file: a manual mutation that fully disabled
  // GlStateGuard's read-fb restore (gl_state.hpp dtor) left GL_READ_FRAMEBUFFER_BINDING==0 after
  // render() regardless, and a zero-sentinel version of this exact check kept PASSING -- a false
  // negative. Binding a genuine non-zero FBO here closes that blind spot.
  // PT: Sentinela de estado do host -- ver os parágrafos EN acima (espelhados aqui em conceito).
  // CRITICAMENTE, o sentinela de READ-framebuffer abaixo usa um FBO REAL, NÃO-ZERO (não 0, o
  // default) -- um sentinela "ainda 0 depois do render()" é um PONTO CEGO comprovado
  // empiricamente ao escrever este arquivo: desligar manualmente a restauração de read-fb do
  // GlStateGuard ainda deixava GL_READ_FRAMEBUFFER_BINDING==0 depois do render() (porque é
  // exatamente o valor que a própria captura deixa), e uma versão com sentinela=0 deste check
  // continuava PASSANDO -- um falso negativo. Um FBO genuinamente não-zero fecha esse ponto cego.
  // ===========================================================================================
  {
    // NOTE: the sentinel texture/unit/read-fbo are bound and re-verified around a THIRD render()
    // call (not reusing run B's already-consumed pixel readback) so this check is unambiguous
    // about which render() call it brackets.
    GLuint sentinel_tex = 0;
    glGenTextures(1, &sentinel_tex);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, sentinel_tex);
    unsigned char pixel[4] = {12, 34, 56, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // EN: A minimal, COMPLETE FBO (renderbuffer colour attachment) so it is a valid bind target,
    //     bound ONLY to GL_READ_FRAMEBUFFER (never GL_FRAMEBUFFER/GL_DRAW_FRAMEBUFFER) -- mirrors
    //     a host that, e.g., is mid-way through its OWN post-processing read step when it hands
    //     control to UiLayer::render() (a realistic embed-mode scenario, not a contrived one).
    // PT: Um FBO mínimo e COMPLETO (anexo de cor via renderbuffer) pra ser um alvo de bind
    //     válido, vinculado SÓ a GL_READ_FRAMEBUFFER -- espelha um host que, por ex., está no meio
    //     do PRÓPRIO passo de leitura de pós-processamento quando entrega controle a
    //     UiLayer::render() (cenário realista de embed mode, não artificial).
    GLuint sentinel_rbo = 0, sentinel_read_fbo = 0;
    glGenRenderbuffers(1, &sentinel_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, sentinel_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 4, 4);
    glGenFramebuffers(1, &sentinel_read_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sentinel_read_fbo);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, sentinel_rbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // EN: Deliberately NOT calling render_once() here -- that helper's OWN glReadPixels prologue
    //     ends with glBindFramebuffer(GL_FRAMEBUFFER, 0), which would unconditionally clobber
    //     BOTH read+draw targets back to 0 immediately AFTER ui.render() returns, defeating the
    //     whole point of the non-zero read-fbo sentinel above (confirmed while writing this file:
    //     using render_once() here made GL_READ_FRAMEBUFFER_BINDING read back as 0 regardless of
    //     whether GlStateGuard restored it correctly or not -- a second blind spot). Call
    //     update()+render() directly instead, so the state checked below is EXACTLY what
    //     UiLayer::render() itself left behind, nothing added after it.
    // PT: Deliberadamente NÃO chama render_once() aqui -- o próprio prólogo glReadPixels daquele
    //     helper termina com glBindFramebuffer(GL_FRAMEBUFFER, 0), que sobrescreveria os dois
    //     alvos read+draw de volta pra 0 logo APÓS ui.render() retornar, anulando o propósito
    //     inteiro do sentinela de read-fbo não-zero acima (confirmado ao escrever este arquivo).
    //     Chama update()+render() direto, então o estado checado abaixo é EXATAMENTE o que
    //     UiLayer::render() deixou, nada adicionado depois.
    ui.update();
    ui.render();  // EN: ripple still active (class not removed) -- mid-frame capture recurs.
                  // PT: ripple ainda ativo (classe não removida) -- captura no meio do frame
                  //     se repete.

    GLint active_tex_after = -1, tex2d_unit5_after = -1;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex_after);
    // EN: GL_TEXTURE_BINDING_2D always queries whichever unit is CURRENTLY active -- must
    //     explicitly re-select GL_TEXTURE5 first (glGetIntegerv alone does not).
    // PT: GL_TEXTURE_BINDING_2D sempre consulta a unidade CORRENTEMENTE ativa -- é preciso
    //     re-selecionar GL_TEXTURE5 explicitamente primeiro.
    glActiveTexture(GL_TEXTURE5);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2d_unit5_after);

    std::printf("ripple_stress_sanity [host sentinel]: GL_ACTIVE_TEXTURE after render = 0x%x "
                "(expect 0x%x = GL_TEXTURE5), GL_TEXTURE_BINDING_2D on unit5 after render = %d "
                "(expect %u = sentinel_tex)\n",
        active_tex_after, GL_TEXTURE5, tex2d_unit5_after, sentinel_tex);
    if (static_cast<GLenum>(active_tex_after) != GL_TEXTURE5) {
      std::puts("ripple_stress_sanity FAIL [SENTINEL-UNIT]: GL_ACTIVE_TEXTURE was not restored to "
                 "the host's own GL_TEXTURE5 after UiLayer::render() -- GlStateGuard's active-"
                 "texture-unit restore is missing or broken");
      ++failures;
    }
    if (static_cast<GLuint>(tex2d_unit5_after) != sentinel_tex) {
      std::puts("ripple_stress_sanity FAIL [SENTINEL-TEX]: the host's own texture bound on "
                 "GL_TEXTURE5 was not restored after UiLayer::render() -- GlStateGuard's "
                 "texture-binding restore is missing or broken");
      ++failures;
    }

    GLint draw_fbo_after = -1, read_fbo_after = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo_after);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo_after);
    std::printf("ripple_stress_sanity [host sentinel]: GL_DRAW_FRAMEBUFFER_BINDING=%d (expect 0), "
                "GL_READ_FRAMEBUFFER_BINDING=%d after render (expect %u = sentinel_read_fbo, "
                "NON-ZERO -- see this block's own top comment on why a zero sentinel would be a "
                "blind spot)\n",
        draw_fbo_after, read_fbo_after, sentinel_read_fbo);
    if (draw_fbo_after != 0) {
      std::puts("ripple_stress_sanity FAIL [SENTINEL-DRAWFBO]: draw framebuffer binding was not "
                 "restored to the host's own FBO 0 after UiLayer::render()");
      ++failures;
    }
    if (static_cast<GLuint>(read_fbo_after) != sentinel_read_fbo) {
      std::printf("ripple_stress_sanity FAIL [SENTINEL-READFBO]: GL_READ_FRAMEBUFFER_BINDING was "
                  "%d, expected the host's own non-zero sentinel FBO %u -- GlStateGuard's "
                  "read-framebuffer restore (L1.22-CAPTURE addition) is missing or broken\n",
          read_fbo_after, sentinel_read_fbo);
      ++failures;
    }

    glDeleteTextures(1, &sentinel_tex);
    glDeleteFramebuffers(1, &sentinel_read_fbo);
    glDeleteRenderbuffers(1, &sentinel_rbo);
  }

  glCopyTexSubImage2D = g_real_copy_tex_sub_image_2d;  // EN: restore, good hygiene.
                                                        // PT: restaura, boa higiene.

  if (failures == 0) {
    std::puts("ripple_stress_sanity: PASS");
    return 0;
  }
  std::printf("ripple_stress_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
