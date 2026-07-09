// SPDX-License-Identifier: MPL-2.0
// EN: image_tint_sanity -- proves the "image-tint(<url>)" luminance-key tint decorator
//     (GLINTFX-TINT-1/2, decorator_image_tint.{hpp,cpp} + the "glintfx-tint" shader injection in
//     render_gl3.cpp, consumer-driven by GusWorld: one neutral base texture, N domain colours at
//     runtime) actually recolours ONLY light+neutral texels while leaving saturated and dark
//     texels alone, is a true no-op at mode=None (pixel-identical to the native image()
//     decorator), degrades fail-high on malformed input (no crash), composites correctly under
//     CSS opacity, reads its tint state FRESH every RenderElement call (not baked at
//     GenerateElementData time), and -- the central risk flagged by ADR-0010 -- never corrupts
//     the NEXT native RmlUi/glintfx draw call in the same frame via a stale `active_program`
//     cache (the ResetProgram() discipline).
//
//     UiLayer/WindowGlfw embed-mode harness (same as polygon_sanity.cpp/
//     polygon_gradient_sanity.cpp) -- runs in BOTH build configs
//     (GLINTFX_BACKEND_GLFW=ON and =OFF): the "image-tint" decorator + the "glintfx-tint" shader
//     injection are both backend-agnostic (registered by Bootstrap, dispatched by
//     Gl3RenderInterface -- always compiled regardless of the GLFW App/window backend option).
//
//     Fixture: tests/ui/image_tint_fixture.png (300x100, generated for this test via PIL, 3
//     equal-width vertical zones):
//       zone A [x=0..100)   #f0f0f0 (240,240,240) -- light + NEUTRAL   ("rune")
//       zone B [x=100..200) #f0c040 (240,192, 64) -- light + SATURATED ("gold trim")
//       zone C [x=200..300) #202020 ( 32, 32, 32) -- dark              ("stone")
//     Fully opaque (alpha=255 everywhere) -- keeps the premultiply un/re-multiply round-trip
//     trivial (divide/multiply by 1.0) so this fixture isolates the luminance-key MATH from any
//     alpha-related edge case.
//
//     Luminance (Rec.601 weights, matching the shader): L = 0.299 R + 0.587 G + 0.114 B.
//       zone A: L = 0.941           (>> threshold 0.5) sat = 0            -> full tint weight
//       zone B: L = 0.752           (>  threshold 0.5) sat = 0.69 (high)  -> LOW tint weight
//       zone C: L = 0.125           (<  threshold 0.5)                   -> ZERO tint weight
//     With image-tint-color=#ff0000 (straight (1,0,0,1)) and mode=luminance-multiply, threshold
//     =0.5, hand-derived expected outputs (see the assertions below for the exact arithmetic):
//       zone A -> strongly RED   (~240,  9,  9)  -- w ~= 0.96
//       zone B -> still GOLD-ish (~240,162, 54)  -- w ~= 0.16 (R unaffected since tint R=1)
//       zone C -> UNCHANGED      ( 32, 32, 32)   -- w  = 0 (smoothstep floors below edge0)
//     This 3-way split (strongly-tinted / barely-tinted / untouched) is the load-bearing proof --
//     it is the only test in this file that proves LUMINANCE-KEY selectivity, not just "some
//     shader ran" (see Proof 1 below).
//
// PT: image_tint_sanity -- prova que o decorator de tingimento luminance-key
//     "image-tint(<url>)" (GLINTFX-TINT-1/2, decorator_image_tint.{hpp,cpp} + a injeção de shader
//     "glintfx-tint" em render_gl3.cpp, consumer-driven pelo GusWorld: uma textura base neutra,
//     N cores de domínio em runtime) de fato recolore SÓ texels claros+neutros deixando texels
//     saturados e escuros intocados, é um no-op verdadeiro em mode=None (pixel-idêntico ao
//     decorator image() nativo), degrada fail-high em entrada malformada (sem crash), compõe
//     corretamente sob opacity do CSS, lê seu estado de tint FRESCO a cada chamada de
//     RenderElement (não embutido no momento de GenerateElementData), e -- o risco central
//     sinalizado pelo ADR-0010 -- nunca corrompe a PRÓXIMA chamada de draw nativa do
//     RmlUi/glintfx no mesmo frame via um cache `active_program` obsoleto (a disciplina de
//     ResetProgram()).
//
//     Harness embed-mode UiLayer/WindowGlfw (igual polygon_sanity.cpp/
//     polygon_gradient_sanity.cpp) -- roda em AMBAS as configs de build.
//
//     Fixture, luminância e valores esperados: ver os parágrafos EN acima (espelhados aqui em
//     conceito): 3 zonas verticais de 100px (clara+neutra / clara+saturada "ouro" / escura
//     "pedra"), totalmente opaca, com a matemática de luminância/peso derivada à mão pro caso
//     luminance-multiply + #ff0000 + threshold=0.5 -- essa divisão em 3 (fortemente tingida /
//     levemente tingida / intocada) é a prova de carga da feature inteira.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"  // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                          // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK) -- same methodology as polygon_sanity.cpp/
//     polygon_gradient_sanity.cpp.
// PT: Renderiza dois frames e lê de volta o FBO 0 (GL_BACK) -- mesma metodologia de
//     polygon_sanity.cpp/polygon_gradient_sanity.cpp.
// ---------------------------------------------------------------------------
static std::vector<unsigned char> capture(glintfx::UiLayer& ui, int w, int h) {
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

// EN: Sample at RCSS content-space coordinates (top-left origin, y-down) -- glReadPixels' row 0
//     is the window BOTTOM, so the row is flipped; column (x) needs no flip. Same convention as
//     polygon_sanity.cpp/polygon_gradient_sanity.cpp.
// PT: Amostra em coordenadas de espaço-conteúdo do RCSS (origem topo-esquerda, y pra baixo) -- a
//     linha 0 do glReadPixels é a BASE da janela, então a linha é invertida; a coluna (x) não
//     precisa de flip. Mesma convenção de polygon_sanity.cpp/polygon_gradient_sanity.cpp.
static const unsigned char* pixel_at(const std::vector<unsigned char>& px, int w, int h, int content_x, int content_y) {
  const int row = h - 1 - content_y;
  const int col = content_x;
  return &px[(static_cast<size_t>(row) * w + col) * 3];
}

int main() {
  constexpr int W = 900, H = 800;

  glintfx::WindowGlfw host;
  if (!host.create("image_tint_sanity_host", W, H)) {
    std::puts("image_tint_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("image_tint_sanity FAIL: ui attach failed");
    return 2;
  }

  // EN: create_data_model -> bind_string -> load() -- required order (Engine enforces it), see
  //     docs/embed-integration.md section 6. Drives #fresh_tint's image-tint-color via
  //     data-style-image-tint-color="tint_color" (image_tint_scene.rml) -- see Proof 7 below.
  // PT: create_data_model -> bind_string -> load() -- ordem obrigatória (o Engine a impõe), ver
  //     docs/embed-integration.md seção 6. Dirige o image-tint-color de #fresh_tint via
  //     data-style-image-tint-color="tint_color" (image_tint_scene.rml) -- ver Prova 7 abaixo.
  const bool ok_model = ui.create_data_model("tint_demo");
  const bool ok_bind = ui.bind_string("tint_color", "red");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  // EN: Every #bad_*/#missing_texture/#empty_src box logs a "decorator ignored"/compile-related
  //     warning by design (fail-high). Expected, not a test failure.
  // PT: Toda caixa #bad_*/#missing_texture/#empty_src loga um aviso de "decorator
  //     ignorado"/relacionado a compile por design (fail-high). Esperado, não é falha de teste.
  const bool ok_load = ui.load("image_tint_scene.rml");

  if (!ok_model || !ok_bind || !ok_load) {
    std::printf("image_tint_sanity FAIL: setup -- model=%d bind=%d load=%d\n", ok_model, ok_bind, ok_load);
    return 3;
  }

  const auto px = capture(ui, W, H);

  int failures = 0;

  // ===========================================================================================
  // Proof 1 -- THE load-bearing luminance-key proof. #luminance box = (20,20,300,100).
  //   Zone centres: A=(20+50,20+50)=(70,70)  B=(170,70)  C=(270,70).
  // ===========================================================================================
  {
    const auto* a = pixel_at(px, W, H, 70, 70);
    const auto* b = pixel_at(px, W, H, 170, 70);
    const auto* c = pixel_at(px, W, H, 270, 70);
    std::printf("image_tint_sanity [luminance]: A=(%d,%d,%d) B=(%d,%d,%d) C=(%d,%d,%d)\n",
        a[0], a[1], a[2], b[0], b[1], b[2], c[0], c[1], c[2]);

    // Zone A: expected ~(240,9,9) -- strongly red, G/B collapsed.
    if (!(a[0] > 190 && a[1] < 60 && a[2] < 60)) {
      std::puts("image_tint_sanity FAIL [1a]: zone A (light+neutral) not strongly tinted red -- "
                 "luminance-key weight wrong for a light/neutral texel");
      ++failures;
    }
    // Zone B (gold): expected ~(240,162,54) -- close to the ORIGINAL gold (240,192,64), NOT
    // anywhere close to zone A's near-pure-red. R must stay high (tint R multiplier=1); G must
    // stay clearly above B (gold's own R>G>B ordering preserved, unlike a fully-tinted texel
    // where G/B would both collapse near 0 together).
    if (!(b[0] > 190 && b[1] > 120 && b[1] > b[2] + 40)) {
      std::puts("image_tint_sanity FAIL [1b]: zone B (light+SATURATED gold) tinted too strongly "
                 "-- luminance-key should weight this DOWN via (1-saturation), leaving it close "
                 "to the original gold");
      ++failures;
    }
    // Zone B must also be CLEARLY less tinted than zone A (the actual selectivity proof, not
    // just an absolute threshold): zone A's G/B collapsed near 0, zone B's must not.
    if (!(b[1] > a[1] + 60)) {
      std::puts("image_tint_sanity FAIL [1b2]: zone B not clearly less tinted than zone A -- "
                 "luminance-key selectivity (weighted by 1-saturation) not demonstrated");
      ++failures;
    }
    // Zone C (dark stone): expected EXACTLY (32,32,32) -- w=0 below the 0.5 threshold, entirely
    // untouched by the tint colour.
    if (!(c[0] > 20 && c[0] < 45 && c[1] > 20 && c[1] < 45 && c[2] > 20 && c[2] < 45 &&
          (c[0] - c[2] > -8) && (c[0] - c[2] < 8))) {
      std::puts("image_tint_sanity FAIL [1c]: zone C (dark) was tinted -- should be fully "
                 "UNCHANGED (luminance below threshold -> zero tint weight)");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 2 -- multiply mode: uniform tint, ALL three zones shift (no luminance-key selectivity).
  //   #multiply box = (20,140,300,100). Same zone centres, offset by +120 in y.
  //   Contrast with Proof 1's zone C: multiply(dark_grey * red) collapses G/B toward 0 (still
  //   red-shifted, unlike luminance-multiply's untouched zone C).
  // ===========================================================================================
  {
    const auto* a = pixel_at(px, W, H, 70, 190);
    const auto* c = pixel_at(px, W, H, 270, 190);
    std::printf("image_tint_sanity [multiply]: A=(%d,%d,%d) C=(%d,%d,%d)\n", a[0], a[1], a[2], c[0], c[1], c[2]);

    if (!(a[0] > 190 && a[1] < 40 && a[2] < 40)) {
      std::puts("image_tint_sanity FAIL [2a]: multiply zone A not tinted red");
      ++failures;
    }
    // The distinguishing proof vs. luminance-multiply: multiply's dark zone C is NOT left alone
    // -- it becomes a dim RED (G/B collapse toward 0), unlike Proof 1c's untouched grey.
    if (!(c[1] < 20 && c[2] < 20)) {
      std::puts("image_tint_sanity FAIL [2c]: multiply zone C (dark) should ALSO shift toward "
                 "the tint colour (G/B collapse) -- multiply has no luminance-key selectivity, "
                 "unlike luminance-multiply");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 3 -- mode=None no-op: pixel-identical (within tolerance) to the native image()
  //   decorator. #none_default=(20,260,300,100), #native_image=(340,260,300,100).
  // ===========================================================================================
  {
    const auto* none_a = pixel_at(px, W, H, 70, 310);
    const auto* native_a = pixel_at(px, W, H, 390, 310);
    std::printf("image_tint_sanity [none vs native]: none=(%d,%d,%d) native=(%d,%d,%d)\n",
        none_a[0], none_a[1], none_a[2], native_a[0], native_a[1], native_a[2]);
    const int dr = none_a[0] - native_a[0], dg = none_a[1] - native_a[1], db = none_a[2] - native_a[2];
    if (!((dr > -6 && dr < 6) && (dg > -6 && dg < 6) && (db > -6 && db < 6))) {
      std::puts("image_tint_sanity FAIL [3]: image-tint-mode:none differs from native image() -- "
                 "the vanilla/no-op path is not truly equivalent");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 4 -- hardening (fail-high, no crash).
  // ===========================================================================================
  {
    // 4a -- unknown keyword falls back to mode=none (RmlUi's own "keyword" parser gate) --
    //   #bad_mode=(20,380,150,60), zone A centre at box-relative (25,30) -> box-local zone width
    //   50px (300px fixture stretched into a 150px box).
    const auto* bad_mode_a = pixel_at(px, W, H, 45, 410);
    std::printf("image_tint_sanity [bad_mode zoneA]: (%d,%d,%d)\n", bad_mode_a[0], bad_mode_a[1], bad_mode_a[2]);
    if (!(bad_mode_a[0] > 190 && bad_mode_a[1] > 190 && bad_mode_a[2] > 190)) {
      std::puts("image_tint_sanity FAIL [4a]: image-tint-mode:bogus-value did not fall back to "
                 "none (zone A should still be light/neutral, not tinted)");
      ++failures;
    }

    // 4b -- threshold=5.0 clamped to 0.999 (NOT 1.0 -- see decorator_image_tint.cpp's
    //   ResolveTintState doc comment: edge0==edge1==1.0 is GLSL smoothstep() UB by spec).
    //   #bad_threshold_over=(190,380,150,60), sampled at box-relative (25,30) -> zone A (same
    //   box-local zone layout as #bad_mode above: 300px fixture stretched into a 150px box, zone
    //   A occupies box-local [0,50)). Zone A is the fixture's BRIGHTEST texel (L=0.941), still
    //   below the clamped 0.999 threshold, so this is now a DEFINED-behaviour assertion, not just
    //   "did not crash": smoothstep(0.999, 1.0, 0.941) floors to 0 (x <= edge0), so the tint
    //   weight is exactly 0 everywhere in this box and it must render fully UNTINTED -- i.e.
    //   close to the fixture's native zone A colour (240,240,240), NOT reddish (image-tint-color
    //   is #ff0000). An extreme/hostile threshold degrades to "no visible tint", not to
    //   NaN/undefined pixels.
    const auto* bad_thr = pixel_at(px, W, H, 215, 410);
    std::printf("image_tint_sanity [bad_threshold_over]: (%d,%d,%d)\n",
        bad_thr[0], bad_thr[1], bad_thr[2]);
    if (!(bad_thr[0] > 200 && bad_thr[1] > 200 && bad_thr[2] > 200)) {
      std::puts("image_tint_sanity FAIL [4b]: threshold=5.0 (clamped to 0.999) should leave zone A "
                 "fully UNTINTED (w=0, smoothstep floors below edge0) -- got a tinted/reddish or "
                 "otherwise implausible pixel instead of the defined all-zero-weight behaviour");
      ++failures;
    }

    // 4c -- missing texture file: DecoratorInstancerInterface::GetTexture resolves structurally
    //   (see decorator_image_tint.cpp's InstanceDecorator finding comment) so the decorator DOES
    //   instantiate; the failure surfaces lazily as an unbound (handle=0) texture, which RmlUi's
    //   OWN RenderGeometry renders as an opaque WHITE quad (ProgramId::Color, fragColor = the
    //   mesh's own opaque-white vertex colour) -- confirmed against
    //   Backends/RmlUi_Renderer_GL3.cpp's RenderGeometry/shader_frag_color. This is the exact
    //   same behaviour the native image() decorator already has for a bad src (no special
    //   casing added here) -- NOT the box's own #303030 background.
    const auto* missing = pixel_at(px, W, H, 435, 410);
    std::printf("image_tint_sanity [missing_texture]: (%d,%d,%d)\n", missing[0], missing[1], missing[2]);
    if (!(missing[0] > 200 && missing[1] > 200 && missing[2] > 200)) {
      std::puts("image_tint_sanity FAIL [4c]: missing-texture box is not the expected "
                 "opaque-white unbound-texture render (ProgramId::Color path) -- unexpected "
                 "behaviour change");
      ++failures;
    }

    // 4d -- empty src ("image-tint();"): InstanceDecorator rejects fail-high (decorator never
    //   instantiated) -- the element's OWN #303030 background must show through, untouched.
    const auto* empty_src = pixel_at(px, W, H, 605, 410);
    std::printf("image_tint_sanity [empty_src]: (%d,%d,%d)\n", empty_src[0], empty_src[1], empty_src[2]);
    if (!(empty_src[0] > 35 && empty_src[0] < 65 && empty_src[1] > 35 && empty_src[1] < 65 && empty_src[2] > 35 && empty_src[2] < 65)) {
      std::puts("image_tint_sanity FAIL [4d]: image-tint() with empty src did not fall back to "
                 "the box's own #303030 background -- decorator was not rejected");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 5 -- opacity interaction. #opacity_half=(20,460,300,100), opacity:0.5, same recipe as
  //   Proof 1's #luminance. NOTE the expected ratio is 0.75, NOT 0.5: capture() (like
  //   polygon_gradient_sanity.cpp's own methodology) renders TWO frames with NO clear between
  //   them (UiLayer's compose-mode contract), so a PARTIALLY-transparent draw accumulates:
  //   frame 1 blends tinted*alpha over black (= tinted*0.5); frame 2 blends the SAME draw again,
  //   this time over frame 1's own result (dst=tinted*0.5) -- giving
  //   tinted*alpha + dst*(1-alpha) = tinted*0.5 + tinted*0.5*0.5 = tinted*0.75. General form:
  //   tinted*(1-(1-alpha)^2). This is a property of the 2-frame no-clear TEST HARNESS (harmless
  //   for opaque content, where it is idempotent), NOT a bug in the tint/opacity shader math --
  //   confirmed empirically: 240*0.75 = 180.0, exactly the observed zone-A red channel below.
  //   The real assertion is simply that opacity CLEARLY dims the result (not ignored, not
  //   doubled into near-invisibility) -- a loose band around the derived 0.75 ratio.
  // PT: Prova 5 -- interação com opacity. #opacity_half=(20,460,300,100), opacity:0.5, mesma
  //   receita da Prova 1's #luminance. NOTE que a razão esperada é 0.75, NÃO 0.5: capture()
  //   (igual a metodologia do próprio polygon_gradient_sanity.cpp) renderiza DOIS frames SEM
  //   clear entre eles (contrato de modo-compose do UiLayer), então um draw PARCIALMENTE
  //   transparente acumula: frame 1 mistura tinted*alpha sobre preto (= tinted*0.5); frame 2
  //   mistura o MESMO draw de novo, desta vez sobre o resultado do frame 1 (dst=tinted*0.5) --
  //   dando tinted*alpha + dst*(1-alpha) = tinted*0.5 + tinted*0.5*0.5 = tinted*0.75. Forma
  //   geral: tinted*(1-(1-alpha)^2). Isso é propriedade do HARNESS de teste de 2-frames-sem-clear
  //   (inofensivo pra conteúdo opaco, onde é idempotente), NÃO um bug na matemática de
  //   shader/opacity -- confirmado empiricamente: 240*0.75 = 180.0, exatamente o canal vermelho
  //   de zona-A observado abaixo. A asserção real é simplesmente que opacity claramente escurece
  //   o resultado (não ignorada, não dobrada até quase-invisibilidade) -- uma banda solta ao
  //   redor da razão 0.75 derivada.
  // ===========================================================================================
  {
    const auto* full_a = pixel_at(px, W, H, 70, 70);   // Proof 1's #luminance zone A (opacity=1).
    const auto* half_a = pixel_at(px, W, H, 70, 510);  // #opacity_half zone A (opacity=0.5).
    std::printf("image_tint_sanity [opacity]: full=(%d,%d,%d) half=(%d,%d,%d)\n",
        full_a[0], full_a[1], full_a[2], half_a[0], half_a[1], half_a[2]);
    const int expected = static_cast<int>(full_a[0] * 0.75f);
    const int diff = half_a[0] > expected ? half_a[0] - expected : expected - half_a[0];
    if (diff > 20) {
      std::puts("image_tint_sanity FAIL [5]: opacity:0.5 tinted element does not composite at "
                 "the expected ~0.75x (2-frame-accumulated) brightness over the black "
                 "background -- opacity not applied correctly");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 6 -- cross-contamination / ResetProgram() state-restore (ADR-0010's central risk).
  //   #polygon_before=(20,580,200,200) centre=(120,680); #tint_before=(240,580,200,200);
  //   #polygon_after=(460,580,200,200) centre=(560,680). Both polygon boxes share the IDENTICAL
  //   radial-gradient(#ffffff,#000000) recipe -- #polygon_before is the "known good" control
  //   FROM THE SAME FRAME; #polygon_after must match it (near-white centre, t=0). If the tinted
  //   draw between them leaves the wrong GL program bound (missing/broken ResetProgram()),
  //   #polygon_after would render with the WRONG uniforms entirely -- a visibly non-white centre.
  //   A single isolated tinted-then-gradient pair (no gradient BEFORE the tint draw) would NOT
  //   catch this -- see render_gl3.cpp's Gl3RenderInterface::RenderShader doc comment.
  // ===========================================================================================
  {
    const auto* before = pixel_at(px, W, H, 120, 680);
    const auto* after = pixel_at(px, W, H, 560, 680);
    std::printf("image_tint_sanity [ResetProgram]: polygon_before=(%d,%d,%d) polygon_after=(%d,%d,%d)\n",
        before[0], before[1], before[2], after[0], after[1], after[2]);
    if (!(before[0] > 200 && before[1] > 200 && before[2] > 200)) {
      std::puts("image_tint_sanity FAIL [6-control]: #polygon_before centre not near-white -- "
                 "control itself broken, cannot validate the ResetProgram() proof");
      ++failures;
    }
    if (!(after[0] > 200 && after[1] > 200 && after[2] > 200)) {
      std::puts("image_tint_sanity FAIL [6]: #polygon_after centre not near-white -- the "
                 "tinted draw between #polygon_before and #polygon_after left the wrong GL "
                 "program bound (this->ResetProgram() missing or broken)");
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 7 -- fresh-per-frame read (ADR-0010 Decision (c)): #fresh_tint's image-tint-color is
  //   driven by data-style-image-tint-color="tint_color" (image_tint_scene.rml), NOT a static
  //   RCSS value. The capture() above used the initial bind_string("tint_color","red"). Now
  //   change it to "blue" via set_string() + update() -- WITHOUT touching the decorator's own
  //   `src` argument, so GenerateElementData does NOT re-run (only RenderElement re-reads the
  //   property, every frame) -- and re-capture. If ResolveTintState were (incorrectly) baked at
  //   GenerateElementData time instead of read fresh in RenderElement, the second capture would
  //   show the SAME (stale, red) tint as the first.
  //   #fresh_tint=(560,20,300,100), zone A centre=(560+50,20+50)=(610,70).
  // ===========================================================================================
  {
    const auto* fresh_red = pixel_at(px, W, H, 610, 70);
    std::printf("image_tint_sanity [fresh, tint_color=red]: A=(%d,%d,%d)\n", fresh_red[0], fresh_red[1], fresh_red[2]);
    if (!(fresh_red[0] > 190 && fresh_red[1] < 60 && fresh_red[2] < 60)) {
      std::puts("image_tint_sanity FAIL [7a]: #fresh_tint zone A not red under the initial "
                 "tint_color=\"red\" data-model value");
      ++failures;
    }

    ui.set_string("tint_color", "blue");
    const auto px2 = capture(ui, W, H);
    const auto* fresh_blue = pixel_at(px2, W, H, 610, 70);
    std::printf("image_tint_sanity [fresh, tint_color=blue]: A=(%d,%d,%d)\n", fresh_blue[0], fresh_blue[1], fresh_blue[2]);
    if (!(fresh_blue[2] > 190 && fresh_blue[0] < 60 && fresh_blue[1] < 60)) {
      std::puts("image_tint_sanity FAIL [7b]: #fresh_tint zone A did not turn blue after "
                 "set_string(\"tint_color\",\"blue\") -- ResolveTintState is stale/baked, not "
                 "reading image-tint-color fresh every RenderElement call");
      ++failures;
    }
  }

  if (failures == 0) {
    std::puts("image_tint_sanity: PASS");
    return 0;
  }
  std::printf("image_tint_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
