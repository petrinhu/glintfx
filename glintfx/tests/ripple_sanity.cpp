// SPDX-License-Identifier: MPL-2.0
// EN: ripple_sanity -- ADVERSARIAL GATE for the "ripple([<max-radius>])" screen-space backdrop
//     refraction decorator (L1.22-WAVE, decorator_ripple.{hpp,cpp} + the "glintfx-ripple" shader
//     injection in render_gl3.cpp) and its FBO-0 backdrop capture (L1.22-CAPTURE,
//     Gl3RenderInterface::CaptureBackdrop, wired from RenderGl3::begin_frame_compose). This file
//     was NOT written by the implementer of L1.22-WAVE/L1.22-CAPTURE (commit 647350f) -- it is
//     the independent, adversarial proof the QA mandate requires: EXECUTE the effect under Xvfb/
//     Mesa-llvmpipe and read back real pixels, do not just re-read the implementer's own claims.
//
//     THE CENTRAL METHODOLOGICAL PROBLEM this file solves: the implementer's own commit message
//     claims "capture only occurs when a ripple is active" (the cost-zero-when-inactive gate,
//     CaptureBackdrop's `if (!ripple_active_counter_ || *ripple_active_counter_ <= 0) return;`).
//     A PIXEL-ONLY test CANNOT falsify that claim: when zero ripple elements exist, disabling the
//     gate (i.e. capturing unconditionally every frame) produces the EXACT SAME rendered output
//     as leaving the gate in place -- nothing in the document samples backdrop_capture_tex_
//     either way, so the composited pixels are bit-identical regardless of whether the capture
//     ran. The gate is a PURE COST optimisation with ZERO pixel-visible effect when inactive --
//     which is exactly what makes it easy to silently regress (e.g. an author "simplifying" the
//     early-return away) without any pixel-diff-based CI ever catching it. Proof 2 below solves
//     this the only way possible without touching src/: it SPIES on the actual
//     glCopyTexSubImage2D GL entry point glintfx calls from inside CaptureBackdrop -- see that
//     Proof's own comment block for the full mechanism and the linkage reasoning that makes it
//     safe (no source file outside tests/ is modified by this technique).
//
//     UiLayer/WindowGlfw embed-mode harness (same host-simulation idiom as image_tint_sanity.cpp/
//     polygon_sanity.cpp), single-frame (NOT the 2-frame-no-clear accumulation trick those two
//     tests use for opacity math) -- render_compose()'s single-composite-pass premultiplied-over
//     blend (`result = sampled_rgb * ring + original_dst * (1 - ring)`) is derived by hand below
//     and needs exactly one frame to hold, not two.
//
//     BACKDROP PATTERN (drawn directly into FBO 0/GL_BACK with raw glScissor+glClear BEFORE the
//     UiLayer ever renders -- UiLayer::render() is compose-only, see engine.cpp's render_compose
//     doc comment, so whatever is already in FBO 0 when it is called is exactly what
//     CaptureBackdrop captures): W=900, H=800, split into two independent, non-overlapping
//     regions so a horizontal-axis leak (zone 1) and a VERTICAL-axis leak (zone 2, the specific
//     axis the commit's own "Y-flip" fix targets -- see kGlintfxRippleFragmentShaderSrc's doc
//     comment in render_gl3.cpp) can both be proven from ONE captured backdrop:
//       Zone 1 (x in [0,450)):   vertical colour boundary at x=225 -- RED  [0,225) | BLUE [225,450)
//       Zone 2 (x in [450,900)): horizontal colour boundary at y=400 -- GREEN [y<400, "top", GL-native
//                                 upper half] | YELLOW [y>=400, "bottom", GL-native lower half]
//     (GL scissor is bottom-left-origin; document/content space -- and this file's own
//     `pixel_at()` -- is top-left-origin/y-down, same convention as every other pixel-reading
//     test in this repo. See draw_pattern()'s own comment for the exact scissor-rectangle-to-
//     document-space derivation.)
//
//     HAND-DERIVED REFRACTION MATH (both zones use IDENTICAL ripple-phase=0, ripple-strength=150,
//     ripple-width=50 -- only ripple-origin-x/-y differ between the two frames rendered below --
//     so the ring/offset arithmetic below is computed ONCE and reused symmetrically for both
//     axes): with ripple-phase=0, front = phase*max_radius = 0 UNCONDITIONALLY (independent of
//     max_radius -- see ripple_scene.rcss's own comment), so `front` in the shader's
//     `ring = 1 - smoothstep(0, width, abs(d - front))` collapses to plain `d` (distance from the
//     ripple-origin). At d=30px (the two sample points this file reads, 30px either side of each
//     zone's own colour boundary):
//       t = d/width = 30/50 = 0.6;  smoothstep(0,50,30) = t^2*(3-2t) = 0.36*1.8 = 0.648
//       ring = 1 - 0.648 = 0.352
//       sin((d-front)*k) = sin(30*0.15) = sin(4.5 rad) ~= -0.97753   (k=0.15, the shader's own
//         hardcoded spatial-frequency constant -- see kGlintfxRippleFragmentShaderSrc)
//       |offset| = ring * strength * |sin(...)| * (1-phase) = 0.352 * 150 * 0.97753 * 1 ~= 51.61px
//     A sample point 30px on the FAR side of the origin from the boundary (i.e. the origin sits
//     ON the boundary, and the sample point is 30px further out) gets pulled BACK ACROSS the
//     boundary by this ~51.6px offset (51.6 > 30) -- see each Proof's own worked-out expected
//     RGB below (premultiplied-over: `result = sampled_rgb*ring + original_dst*(1-ring)`, i.e.
//     `original*0.648 + crossed_colour*0.352`).
// PT: ripple_sanity -- GATE ADVERSARIAL para o decorator de refração de backdrop em espaço de
//     tela "ripple([<raio-max>])" (L1.22-WAVE) e sua captura de backdrop do FBO-0 (L1.22-CAPTURE).
//     Este arquivo NÃO foi escrito pelo implementador do L1.22-WAVE/L1.22-CAPTURE (commit
//     647350f) -- é a prova independente e adversarial que o mandato de QA exige: EXECUTAR o
//     efeito sob Xvfb/Mesa-llvmpipe e ler pixels reais de volta, não só reler as próprias
//     alegações do implementador.
//
//     O PROBLEMA METODOLÓGICO CENTRAL que este arquivo resolve: a própria mensagem de commit do
//     implementador alega "a captura só ocorre quando um ripple está ativo" (o gate de
//     custo-zero-quando-inativo). Um teste SÓ-DE-PIXEL NÃO CONSEGUE falsificar essa alegação:
//     quando zero elementos ripple existem, desligar o gate (isto é, capturar incondicionalmente
//     todo frame) produz a MESMA saída renderizada EXATA de deixar o gate no lugar -- nada no
//     documento amostra backdrop_capture_tex_ de qualquer forma, então os pixels compostos são
//     bit-idênticos independente de a captura ter rodado. O gate é uma otimização PURA DE CUSTO
//     com efeito ZERO visível em pixel quando inativo -- exatamente o que o torna fácil de
//     regredir silenciosamente sem que teste algum baseado em pixel-diff jamais capture. A Prova
//     2 abaixo resolve isso da única forma possível sem tocar src/: ESPIONA o entry point GL real
//     glCopyTexSubImage2D que a glintfx chama de dentro de CaptureBackdrop -- ver o próprio bloco
//     de comentário daquela Prova pro mecanismo completo.
//
//     Harness embed-mode UiLayer/WindowGlfw, frame ÚNICO (NÃO o truque de acumulação de 2-frames-
//     sem-clear que image_tint_sanity.cpp/polygon_sanity.cpp usam para matemática de opacity) --
//     o blend premultiplied-over de UM ÚNICO passe de composição do render_compose() é derivado à
//     mão abaixo e precisa de exatamente um frame para valer, não dois.
//
//     PADRÃO DE BACKDROP: ver os parágrafos EN acima (espelhados aqui em conceito) -- W=900,
//     H=800, duas zonas independentes e não-sobrepostas: zona 1 (fronteira vertical de cor em
//     x=225, RED|BLUE) prova o vazamento no eixo HORIZONTAL; zona 2 (fronteira horizontal de cor
//     em y=400, GREEN|YELLOW) prova o vazamento no eixo VERTICAL -- exatamente o eixo que o
//     próprio fix de "flip em Y" do commit visa (ver o doc-comment de
//     kGlintfxRippleFragmentShaderSrc em render_gl3.cpp).
//
//     MATEMÁTICA DE REFRAÇÃO DERIVADA À MÃO: ver os parágrafos EN acima (espelhados aqui em
//     conceito) -- ring=0.352, |offset|~=51.61px em d=30px, ambos os eixos usando os mesmos
//     ripple-phase=0/ripple-strength=150/ripple-width=50 (só ripple-origin-x/-y mudam entre os
//     dois frames renderizados abaixo).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"  // EN: includes gl_loader.h (GL already loaded by WindowGlfw::create).
                          // PT: inclui gl_loader.h (GL já carregado por WindowGlfw::create).
#include <cstdio>
#include <vector>

namespace {

constexpr int W = 900, H = 800;

// ===============================================================================================
// EN: Proof 2's spy mechanism -- see this file's own top comment for WHY a spy (not a pixel
//     check) is the only thing that can falsify the cost-zero-when-inactive gate.
//
//     LINKAGE REASONING (why this is safe / why it actually intercepts the SAME call
//     render_gl3.cpp makes, without touching any file outside tests/): `glCopyTexSubImage2D` is a
//     plain, non-`static`, non-`const` C global variable of function-pointer type
//     (`PFNGLCOPYTEXSUBIMAGE2DPROC glCopyTexSubImage2D;`), DEFINED in gl_loader.c and populated
//     once at runtime by glx_gl_load() (WindowGlfw::create(), already called by the time main()
//     reaches this file's code). glintfx/CMakeLists.txt documents `glloader` as an INTERFACE
//     library whose "single source (gl_loader.c) is compiled" separately INTO EVERY CONSUMING
//     TARGET (mirrors the old vendored gl3w idiom) -- this test target links BOTH `glintfx`
//     (the STATIC library, which privately compiles its OWN copy of gl_loader.c internally,
//     containing the exact `glCopyTexSubImage2D` global CaptureBackdrop calls) AND `glloader`
//     directly (this file's own gl_loader.h include needs the declarations). Because this test's
//     own translation units are linked BEFORE the linker searches glintfx.a for still-undefined
//     symbols, and gl_loader.c defines EVERY SINGLE GL function-pointer global glintfx.a's other
//     object files (render_gl3.cpp.o, decorator_ripple.cpp.o, ...) reference as `extern` -- this
//     test's own gl_loader.c.o already satisfies every one of those references before
//     glintfx.a's OWN internal gl_loader.c.o member would ever need to be pulled from the
//     archive, so it never is. The upshot: there is exactly ONE `glCopyTexSubImage2D` global
//     variable in the final `ripple_sanity` binary, and BOTH this file's `main()` AND
//     render_gl3.cpp's `CaptureBackdrop` read/call through it -- this is the identical technique
//     several existing tests already rely on for the inverse purpose (golden_test/
//     ui_layer_compose/gl_state_guard link `glintfx glloader` together specifically so their OWN
//     GL calls land in the SAME context/symbol set the library uses; this file additionally
//     exploits that shared symbol to intercept, not just call, one specific GL entry point).
//
//     MUTATION-TESTING CROSS-CHECK (performed manually during this gate's review, NOT baked into
//     this file -- see this file's own review report): temporarily inverting CaptureBackdrop's
//     gate condition (`if (!ripple_active_counter_ || *ripple_active_counter_ <= 0) return;` -->
//     an unconditional capture) and rebuilding makes Proof 2's "inactive" assertion below FAIL
//     (spy count becomes >0 with zero ripple elements in the document) -- confirming this spy
//     actually observes the gate's real behaviour, not a tautology.
// PT: Mecanismo de espião da Prova 2 -- ver o próprio comentário do topo deste arquivo pro
//     POR QUE um espião (não um check de pixel) é a única coisa capaz de falsificar o gate de
//     custo-zero-quando-inativo.
//
//     RACIONAL DE LINKAGEM (por que isto é seguro / por que de fato intercepta a MESMA chamada
//     que render_gl3.cpp faz, sem tocar nenhum arquivo fora de tests/): ver os parágrafos EN
//     acima (espelhados aqui em conceito) -- `glCopyTexSubImage2D` é uma única variável global C
//     de tipo ponteiro-de-função, não-`static`, definida em gl_loader.c e populada uma vez em
//     runtime; como `glloader` é uma biblioteca INTERFACE compilada por-consumidor, e este alvo
//     de teste linka `glintfx glloader` (mesmo idioma de golden_test/ui_layer_compose/
//     gl_state_guard), existe exatamente UMA variável `glCopyTexSubImage2D` no binário final, e
//     tanto este `main()` quanto o `CaptureBackdrop` de render_gl3.cpp leem/chamam através dela.
//
//     CONTRAPROVA DE MUTATION TESTING (feita manualmente durante o review deste gate, NÃO
//     embutida neste arquivo): inverter temporariamente a condição do gate de CaptureBackdrop e
//     rebuildar faz a asserção "inativo" da Prova 2 abaixo FALHAR (contador do espião fica >0 com
//     zero elementos ripple no documento) -- confirmando que este espião de fato observa o
//     comportamento real do gate, não uma tautologia.
// ===============================================================================================
int g_capture_calls = 0;
PFNGLCOPYTEXSUBIMAGE2DPROC g_real_copy_tex_sub_image_2d = nullptr;

void APIENTRY SpyCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLint x, GLint y, GLsizei width, GLsizei height) {
  ++g_capture_calls;
  if (g_real_copy_tex_sub_image_2d)
    g_real_copy_tex_sub_image_2d(target, level, xoffset, yoffset, x, y, width, height);
}

// EN: Draws the two-zone backdrop pattern (see this file's top comment) straight into FBO 0/
//     GL_BACK via raw glScissor+glClear -- no shaders, no geometry, so this is a source of truth
//     wholly independent of glintfx's own GL code path (a bug shared between this harness and
//     the code under test could otherwise mask itself).
//     Scissor-rectangle derivation (GL is bottom-left-origin; document/content space used by
//     pixel_at() below and by ripple-origin-y is top-left-origin/y-down -- same "row = h-1-y"
//     convention every other pixel-reading test in this repo already documents):
//       Zone 1 base fill  x=[0,450)          RED   (whole left half)
//       Zone 1 overwrite  x=[225,450)        BLUE  (narrower -- leaves x=[0,225)=RED, [225,450)=BLUE)
//       Zone 2 base fill  x=[450,900)        GREEN (whole right half -- becomes the visual TOP)
//       Zone 2 overwrite  x=[450,900), GL y=[0,400) YELLOW (GL-native BOTTOM rows = document y>=400,
//         since document_y = H-1-gl_y --> gl_y in [0,400) <=> document_y in (399,799], i.e. the
//         document-space "bottom" half) -- leaves GL y=[400,800) (document y<400, "top") = GREEN.
// PT: Desenha o padrão de backdrop de duas zonas direto no FBO 0/GL_BACK via glScissor+glClear
//     crus -- ver os parágrafos EN acima pra derivação exata dos retângulos de scissor.
void draw_pattern() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, W, H);
  glDisable(GL_SCISSOR_TEST);

  glEnable(GL_SCISSOR_TEST);
  glScissor(0, 0, 450, H);
  glClearColor(1.f, 0.f, 0.f, 1.f);  // RED
  glClear(GL_COLOR_BUFFER_BIT);

  glScissor(225, 0, 225, H);
  glClearColor(0.f, 0.f, 1.f, 1.f);  // BLUE
  glClear(GL_COLOR_BUFFER_BIT);

  glScissor(450, 0, 450, H);
  glClearColor(0.f, 1.f, 0.f, 1.f);  // GREEN
  glClear(GL_COLOR_BUFFER_BIT);

  glScissor(450, 0, 450, 400);
  glClearColor(1.f, 1.f, 0.f, 1.f);  // YELLOW
  glClear(GL_COLOR_BUFFER_BIT);

  glDisable(GL_SCISSOR_TEST);
}

// EN: Single-frame (NOT 2-frame-accumulated, see top comment) update+render+readback of FBO 0.
// PT: update+render+readback de frame ÚNICO (NÃO acumulado em 2 frames, ver comentário do topo)
//     do FBO 0.
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

// EN: Sample at document/content-space coordinates (top-left origin, y-down) -- same "row =
//     h-1-content_y" convention as image_tint_sanity.cpp/polygon_sanity.cpp's own pixel_at().
// PT: Amostra em coordenadas de espaço-conteúdo (origem topo-esquerda, y pra baixo) -- mesma
//     convenção "row = h-1-content_y" do próprio pixel_at() de image_tint_sanity.cpp/
//     polygon_sanity.cpp.
const unsigned char* pixel_at(const std::vector<unsigned char>& px, int content_x, int content_y) {
  const int row = H - 1 - content_y;
  return &px[(static_cast<size_t>(row) * W + content_x) * 3];
}

bool near(int v, int expect, int tol) { return v > expect - tol && v < expect + tol; }

}  // namespace

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("ripple_sanity_host", W, H)) {
    std::puts("ripple_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("ripple_sanity FAIL: ui attach failed");
    return 2;
  }

  // EN: Install the spy AFTER every glx_gl_load() call this process makes -- host.create()
  //     above AND UiLayer's own ctor (cfg.load_gl=true) BOTH call it, and glx_gl_load() is
  //     "idempotent" only in the sense of always resolving to the SAME correct address, NOT in
  //     the sense of skipping already-populated slots: it unconditionally re-resolves and
  //     re-assigns EVERY GL function-pointer global on EVERY call (gl_loader.c's glx_gl_load(),
  //     `*glintfx_gl_symbol_table[i].slot = resolved;`, no "already loaded" early-out). Installing
  //     the spy BEFORE constructing UiLayer would get silently clobbered by UiLayer's own
  //     internal glx_gl_load() call -- confirmed by hitting exactly this bug while writing this
  //     test (spy counted 0 while the SECOND active frame's pixels still showed correct
  //     refraction, proving the REAL glCopyTexSubImage2D was still being called under an
  //     un-spied pointer).
  // PT: Instala o espião DEPOIS de toda chamada a glx_gl_load() que este processo faz --
  //     host.create() acima E o próprio ctor da UiLayer (cfg.load_gl=true) AMBOS o chamam, e
  //     glx_gl_load() é "idempotente" só no sentido de sempre resolver pro MESMO endereço
  //     correto, NÃO no sentido de pular slots já populados. Instalar o espião ANTES de
  //     construir a UiLayer seria silenciosamente sobrescrito pela própria chamada interna de
  //     glx_gl_load() da UiLayer -- confirmado ao bater exatamente neste bug ao escrever este
  //     teste (espião contou 0 enquanto os pixels do SEGUNDO frame ativo ainda mostravam
  //     refração correta, provando que o glCopyTexSubImage2D REAL ainda estava sendo chamado sob
  //     um ponteiro não-espionado).
  g_real_copy_tex_sub_image_2d = glCopyTexSubImage2D;
  glCopyTexSubImage2D = &SpyCopyTexSubImage2D;

  draw_pattern();

  const bool ok_load = ui.load("ripple_scene.rml");
  if (!ok_load) {
    std::puts("ripple_sanity FAIL: load failed");
    return 3;
  }

  int failures = 0;

  // ===========================================================================================
  // Proof 2a -- COST-ZERO-WHEN-INACTIVE (the load-bearing L1.22-CAPTURE proof, see this file's
  //   top comment for why a spy, not a pixel check, is required). #overlay carries NO "active"
  //   class yet -- zero "ripple()" decorators exist anywhere in the document.
  // ===========================================================================================
  {
    g_capture_calls = 0;
    const auto px = render_once(ui);
    std::printf("ripple_sanity [inactive]: CaptureBackdrop-equivalent GL calls = %d\n", g_capture_calls);
    if (g_capture_calls != 0) {
      std::puts("ripple_sanity FAIL [2a]: glCopyTexSubImage2D was called with ZERO active ripple "
                 "decorators in the document -- the cost-zero-when-inactive gate did not fire "
                 "(CaptureBackdrop is running its capture unconditionally)");
      ++failures;
    }

    // Belt-and-suspenders pixel check: with no decorator at all, the composed frame must be
    // BIT-IDENTICAL to the raw pattern (not just "close") -- confirms #overlay's presence in the
    // DOM (with no active decorator) has literally zero rendering side effect.
    struct { int x, y; unsigned char r, g, b; const char* name; } expect[] = {
        {100, 400, 255, 0, 0, "zone1 red"},
        {350, 400, 0, 0, 255, "zone1 blue"},
        {675, 100, 0, 255, 0, "zone2 green"},
        {675, 700, 255, 255, 0, "zone2 yellow"},
    };
    for (const auto& e : expect) {
      const auto* p = pixel_at(px, e.x, e.y);
      if (p[0] != e.r || p[1] != e.g || p[2] != e.b) {
        std::printf("ripple_sanity FAIL [2a-pixel]: %s at (%d,%d) = (%d,%d,%d), expected exactly "
                     "(%d,%d,%d) -- inactive #overlay altered the backdrop\n",
            e.name, e.x, e.y, p[0], p[1], p[2], e.r, e.g, e.b);
        ++failures;
      }
    }
  }

  // Activate the ripple decorator (Engine::add_class idiom, same as domrw_sanity.cpp) and drive
  // the five ripple-* properties via set_property -- see ripple_scene.rcss's own comment for why
  // this keeps every capture below fully deterministic (no @keyframes/transition ever runs).
  const bool ok_active = ui.add_class("overlay", "active");
  const bool ok_phase = ui.set_property("overlay", "ripple-phase", "0");
  const bool ok_strength = ui.set_property("overlay", "ripple-strength", "150");
  const bool ok_width = ui.set_property("overlay", "ripple-width", "50");
  if (!ok_active || !ok_phase || !ok_strength || !ok_width) {
    std::printf("ripple_sanity FAIL: activation setup -- active=%d phase=%d strength=%d width=%d\n",
        ok_active, ok_phase, ok_strength, ok_width);
    return 4;
  }

  // ===========================================================================================
  // Proof 1 (part A) -- HORIZONTAL-axis capture+refraction correctness. ripple-origin=(225,400),
  //   ON zone 1's own RED|BLUE boundary. See this file's top comment for the full hand-derived
  //   math (ring=0.352, |offset|~=51.61px at d=30).
  // ===========================================================================================
  {
    if (!ui.set_property("overlay", "ripple-origin-x", "225") ||
        !ui.set_property("overlay", "ripple-origin-y", "400")) {
      std::puts("ripple_sanity FAIL: set_property ripple-origin (zone1) rejected");
      return 5;
    }

    g_capture_calls = 0;
    const auto px = render_once(ui);
    std::printf("ripple_sanity [zone1 active]: CaptureBackdrop-equivalent GL calls = %d\n", g_capture_calls);
    if (g_capture_calls < 1) {
      std::puts("ripple_sanity FAIL [2b]: glCopyTexSubImage2D was NEVER called with an active "
                 "ripple() decorator present -- CaptureBackdrop's capture did not run at all "
                 "(nothing for the ripple shader to sample -- would render as fully transparent/ "
                 "unaffected backdrop, masking the bug from Proof 1 below unless caught here)");
      ++failures;
    }

    // (255,400): blue side, 30px right of the x=225 boundary -- offset pulls the sample LEFT
    // across the boundary into red territory. Expected ~(90,0,165): R rises from 0, B falls from
    // 255 -- a genuine cross-boundary leak, not a uniform tint.
    const auto* a = pixel_at(px, 255, 400);
    std::printf("ripple_sanity [zone1 horiz A @255,400]: (%d,%d,%d) expect ~(90,0,165)\n", a[0], a[1], a[2]);
    if (!(near(a[0], 90, 30) && a[1] < 15 && near(a[2], 165, 30))) {
      std::puts("ripple_sanity FAIL [1a]: no red leak detected on the blue side of the x=225 "
                 "boundary -- horizontal refraction/capture is wrong or absent");
      ++failures;
    }

    // (195,400): red side, 30px left of the boundary -- mirror-symmetric blue leak.
    const auto* b = pixel_at(px, 195, 400);
    std::printf("ripple_sanity [zone1 horiz B @195,400]: (%d,%d,%d) expect ~(165,0,90)\n", b[0], b[1], b[2]);
    if (!(near(b[0], 165, 30) && b[1] < 15 && near(b[2], 90, 30))) {
      std::puts("ripple_sanity FAIL [1b]: no blue leak detected on the red side of the x=225 "
                 "boundary -- horizontal refraction/capture is wrong or absent, or asymmetric "
                 "(only one direction leaking) -- a real sign-flip bug in the shader's `dir` term "
                 "would show exactly this asymmetry");
      ++failures;
    }

    // Backdrop-fidelity controls -- FAR from the ring (d=175 >> width=50, ring=0 exactly): must
    // be PIXEL-EXACT, proving the capture is faithful (not repainting/darkening the whole zone).
    const auto* red_ctrl = pixel_at(px, 50, 400);
    const auto* blue_ctrl = pixel_at(px, 400, 400);
    std::printf("ripple_sanity [zone1 controls]: red=(%d,%d,%d) blue=(%d,%d,%d)\n",
        red_ctrl[0], red_ctrl[1], red_ctrl[2], blue_ctrl[0], blue_ctrl[1], blue_ctrl[2]);
    if (!(red_ctrl[0] == 255 && red_ctrl[1] == 0 && red_ctrl[2] == 0)) {
      std::puts("ripple_sanity FAIL [1c]: far-from-ring red control was altered -- capture is "
                 "repainting outside the ring band (ring=0 region not fully transparent)");
      ++failures;
    }
    if (!(blue_ctrl[0] == 0 && blue_ctrl[1] == 0 && blue_ctrl[2] == 255)) {
      std::puts("ripple_sanity FAIL [1d]: far-from-ring blue control was altered -- capture is "
                 "repainting outside the ring band (ring=0 region not fully transparent)");
      ++failures;
    }

    // Zone 2 must be completely untouched by zone 1's ripple element (single fullscreen overlay,
    // but the ring is spatially localised around origin=(225,400) -- d to any zone-2 point is
    // >>width).
    const auto* z2_green = pixel_at(px, 675, 100);
    const auto* z2_yellow = pixel_at(px, 675, 700);
    if (!(z2_green[0] == 0 && z2_green[1] == 255 && z2_green[2] == 0) ||
        !(z2_yellow[0] == 255 && z2_yellow[1] == 255 && z2_yellow[2] == 0)) {
      std::printf("ripple_sanity FAIL [1e]: zone2 green=(%d,%d,%d) yellow=(%d,%d,%d) altered by "
                   "zone1's ripple -- ring is not spatially localised as expected\n",
          z2_green[0], z2_green[1], z2_green[2], z2_yellow[0], z2_yellow[1], z2_yellow[2]);
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 1 (part B) -- VERTICAL-axis capture+refraction correctness (the specific axis the
  //   commit's own Y-flip fix targets -- see kGlintfxRippleFragmentShaderSrc's doc comment,
  //   render_gl3.cpp: "amostragem do backdrop precisava de flip vertical"). Re-pointing the SAME
  //   single ripple element's origin at zone 2's GREEN|YELLOW boundary (675,400) -- fresh-per-
  //   frame property read (RippleDecorator::RenderElement calls ResolveRippleState every call,
  //   same discipline as decorator_image_tint.cpp), no re-load needed.
  //   If the Y-flip were missing/wrong, this test's own top-comment derivation shows the sampled
  //   document-y would land at ~422 (still inside the YELLOW half) instead of the correct ~378
  //   (inside GREEN) -- i.e. the "leak" below would NOT be detected; a Y-flip regression
  //   degrades to "looks like nothing happened", not a crash, which is exactly why this needs an
  //   explicit pixel proof, not just "did not crash".
  // ===========================================================================================
  {
    if (!ui.set_property("overlay", "ripple-origin-x", "675") ||
        !ui.set_property("overlay", "ripple-origin-y", "400")) {
      std::puts("ripple_sanity FAIL: set_property ripple-origin (zone2) rejected");
      return 6;
    }

    g_capture_calls = 0;
    const auto px = render_once(ui);
    std::printf("ripple_sanity [zone2 active]: CaptureBackdrop-equivalent GL calls = %d\n", g_capture_calls);
    if (g_capture_calls < 1) {
      std::puts("ripple_sanity FAIL [2c]: glCopyTexSubImage2D was not called for the second "
                 "(zone2) frame -- capture must run every frame an active ripple() is present, "
                 "not just the first");
      ++failures;
    }

    // (675,430): yellow side (document y=430 >= 400), 30px below the y=400 boundary -- offset
    // pulls the sample UP (toward smaller document-y) across the boundary into green territory.
    // Expected ~(165,255,0): R falls from 255 (green leak diluting the yellow), G stays ~255
    // (both source colours share G=255 -- see this file's top comment on why R, not G, is the
    // discriminating channel here).
    const auto* c = pixel_at(px, 675, 430);
    std::printf("ripple_sanity [zone2 vert C @675,430]: (%d,%d,%d) expect ~(165,255,0)\n", c[0], c[1], c[2]);
    if (!(near(c[0], 165, 30) && c[1] > 230 && c[2] < 15)) {
      std::puts("ripple_sanity FAIL [1f]: no green leak detected below the y=400 boundary -- "
                 "VERTICAL refraction is wrong or absent -- this is the specific failure mode a "
                 "missing/incorrect Y-flip on the backdrop texture sample would produce (the "
                 "sampled row would stay on the SAME side of the boundary instead of crossing it, "
                 "degrading silently to \"no visible leak\" rather than a crash)");
      ++failures;
    }

    // (675,370): green side (document y=370 < 400), mirror-symmetric yellow leak.
    const auto* d = pixel_at(px, 675, 370);
    std::printf("ripple_sanity [zone2 vert D @675,370]: (%d,%d,%d) expect ~(90,255,0)\n", d[0], d[1], d[2]);
    if (!(near(d[0], 90, 30) && d[1] > 230 && d[2] < 15)) {
      std::puts("ripple_sanity FAIL [1g]: no yellow leak detected above the y=400 boundary -- "
                 "vertical refraction wrong/absent or asymmetric (same Y-flip risk as 1f)");
      ++failures;
    }

    // Backdrop-fidelity controls, far from the (now-relocated) ring.
    const auto* green_ctrl = pixel_at(px, 675, 50);
    const auto* yellow_ctrl = pixel_at(px, 675, 750);
    std::printf("ripple_sanity [zone2 controls]: green=(%d,%d,%d) yellow=(%d,%d,%d)\n",
        green_ctrl[0], green_ctrl[1], green_ctrl[2], yellow_ctrl[0], yellow_ctrl[1], yellow_ctrl[2]);
    if (!(green_ctrl[0] == 0 && green_ctrl[1] == 255 && green_ctrl[2] == 0)) {
      std::puts("ripple_sanity FAIL [1h]: far-from-ring green control was altered");
      ++failures;
    }
    if (!(yellow_ctrl[0] == 255 && yellow_ctrl[1] == 255 && yellow_ctrl[2] == 0)) {
      std::puts("ripple_sanity FAIL [1i]: far-from-ring yellow control was altered");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 4 -- READ-FRAMEBUFFER RESTORE (GlStateGuard's L1.22-CAPTURE addition, gl_state.hpp).
  //   After render_once() above (which internally invokes CaptureBackdrop's
  //   glBindFramebuffer(GL_READ_FRAMEBUFFER,0)+glReadBuffer(GL_BACK), deliberately un-restored by
  //   CaptureBackdrop itself -- see that function's own doc comment), the READ framebuffer
  //   binding observed from OUTSIDE UiLayer::render() must be back to whatever this test itself
  //   had bound (0 -- this test never binds any other read-fb) -- proving GlStateGuard's dtor
  //   (engine.cpp's render_compose) actually restores it, not just that the visible pixels happen
  //   to look right.
  // ===========================================================================================
  {
    GLint read_fbo = -1;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);
    std::printf("ripple_sanity [read-fbo restore]: GL_READ_FRAMEBUFFER_BINDING after render = %d\n", read_fbo);
    if (read_fbo != 0) {
      std::puts("ripple_sanity FAIL [4]: GL_READ_FRAMEBUFFER_BINDING was not restored to 0 after "
                 "UiLayer::render() -- GlStateGuard's read-framebuffer restore (L1.22-CAPTURE "
                 "addition, gl_state.hpp) is missing or broken");
      ++failures;
    }
  }

  glCopyTexSubImage2D = g_real_copy_tex_sub_image_2d;  // EN: restore, good hygiene.
                                                        // PT: restaura, boa higiene.

  if (failures == 0) {
    std::puts("ripple_sanity: PASS");
    return 0;
  }
  std::printf("ripple_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
