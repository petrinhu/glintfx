// SPDX-License-Identifier: MPL-2.0
// EN: app_font_face_sanity -- App-PATH FORWARD-COVERAGE gate for UI-FONTFACE (v0.24.0, review
//     follow-up). `tests/fontface_load_sanity.cpp` exercises `UiLayer::load_font_face` end-to-
//     end six ways, but NEVER touches `App::load_font_face` -- `App::load_font_face` and
//     `UiLayer::load_font_face` are two SEPARATE method bodies (`src/app.cpp`/`src/ui_layer.cpp`)
//     that both happen to forward to the same `Engine::load_font_face`, so a broken forward on
//     the `App` side specifically (e.g. `App::load_font_face` always returning `false` without
//     ever calling `impl_->engine.load_font_face()`) is INVISIBLE to the UiLayer-only suite.
//     CONFIRMED EXPLOITABLE by the adversarial review (`w18-rev`): that exact mutation --
//     `App::load_font_face` short-circuited to unconditional `false` -- was applied and the
//     FULL `GLINTFX_BACKEND_GLFW=ON` suite still passed 99/100 (the lone failure was a
//     pre-existing, unrelated window-mode flake). This file is the test that turns that
//     mutation into a `FAIL`.
//
//     Three checks, mirroring `fontface_load_sanity.cpp`'s [1]/[2] shape but through the `App`
//     facade specifically:
//     [A1] VALID REGISTRATION + RENDER -- `App::load_font_face` on a real font file with an
//          explicit family returns `true`, AND the text actually assigned that family renders
//          (non-blank ink via `App::snapshot()` + a PPM readback, the same oracle
//          `fonteng_own_sanity.cpp` uses for `App` -- no `glloader` link needed). A stub that
//          returns `true` without forwarding would still fail this: the snapshot would come
//          back blank (no font ever registered with the underlying `Rml::FontEngineInterface`),
//          so BOTH the boolean AND the rendered effect must be correct for this to pass -- kills
//          "always return false" AND a hypothetical "always return true, no-op" mutation alike.
//     [A2] HOSTILE INPUT -- `path=nullptr` returns `false`, no crash (`AUD-TEC-5`, same
//          discipline as every other guarded method reached through `App`).
//     [A3] SANITY CROSS-CHECK -- `App::ok()` must be `true` before either of the above runs;
//          this is the same "no display / GL context" `SKIP` (not `FAIL`) escape hatch every
//          other `App`-based test in this suite uses (`fonteng_own_sanity.cpp`, `app_smoke.cpp`).
//
//     Deliberately narrow in scope (does NOT repeat the A/B-engine / derived-family / fallback_
//     face criteria [3]-[5] already covered end-to-end via `UiLayer` in
//     `fontface_load_sanity.cpp` -- `App::load_font_face` and `UiLayer::load_font_face` share
//     the identical `Engine::load_font_face` implementation underneath, so re-proving the
//     engine-internal behaviour a second time through `App` would be redundant coverage, not a
//     new mutation-killing property; only the `App`-SPECIFIC forwarding wire is what was
//     unverified).
// PT: app_font_face_sanity -- gate de COBERTURA DE FORWARD do caminho App pro UI-FONTFACE
//     (v0.24.0, follow-up de review). `tests/fontface_load_sanity.cpp` exercita
//     `UiLayer::load_font_face` ponta-a-ponta de seis jeitos, mas NUNCA toca
//     `App::load_font_face` -- `App::load_font_face` e `UiLayer::load_font_face` são DOIS
//     corpos de método SEPARADOS (`src/app.cpp`/`src/ui_layer.cpp`) que ambos por acaso
//     encaminham ao mesmo `Engine::load_font_face`, então um forward quebrado especificamente
//     do lado `App` (ex.: `App::load_font_face` sempre retornando `false` sem nunca chamar
//     `impl_->engine.load_font_face()`) é INVISÍVEL pra suíte só-UiLayer. CONFIRMADO
//     EXPLORÁVEL pelo review adversarial (`w18-rev`): exatamente essa mutação --
//     `App::load_font_face` curto-circuitado pra `false` incondicional -- foi aplicada e a
//     suíte COMPLETA com `GLINTFX_BACKEND_GLFW=ON` ainda passou 99/100 (a única falha era um
//     flake pré-existente de window-mode, sem relação). Este arquivo é o teste que transforma
//     essa mutação num `FAIL`.
//
//     Três cheques, espelhando o formato [1]/[2] de `fontface_load_sanity.cpp` mas
//     especificamente pela fachada `App`:
//     [A1] REGISTRO VÁLIDO + RENDER -- `App::load_font_face` num arquivo de fonte real com
//          família explícita retorna `true`, E o texto de fato atribuído àquela família
//          renderiza (tinta não-branca via `App::snapshot()` + leitura de PPM, o mesmo oráculo
//          que `fonteng_own_sanity.cpp` usa pro `App` -- sem precisar linkar `glloader`). Um
//          stub que retorna `true` sem encaminhar ainda falharia isto: o snapshot voltaria em
//          branco (nenhuma fonte jamais registrada na `Rml::FontEngineInterface` subjacente),
//          então TANTO o booleano QUANTO o efeito renderizado precisam estar corretos pra isto
//          passar -- mata "sempre retorna false" E uma hipotética mutação "sempre retorna true,
//          no-op" igualmente.
//     [A2] ENTRADA HOSTIL -- `path=nullptr` retorna `false`, sem crash (`AUD-TEC-5`, mesma
//          disciplina de todo outro método guardado alcançado através do `App`).
//     [A3] CONTRACHEQUE DE SANIDADE -- `App::ok()` precisa ser `true` antes de qualquer um dos
//          acima rodar; esta é a mesma saída de escape `SKIP` (não `FAIL`) de "sem display/
//          contexto GL" que todo outro teste baseado em `App` desta suíte usa
//          (`fonteng_own_sanity.cpp`, `app_smoke.cpp`).
//
//     Deliberadamente estreito de escopo (NÃO repete os critérios [3]-[5] de A/B-de-motor/
//     família-derivada/fallback_face já cobertos ponta-a-ponta via `UiLayer` em
//     `fontface_load_sanity.cpp` -- `App::load_font_face` e `UiLayer::load_font_face`
//     compartilham a implementação `Engine::load_font_face` idêntica por baixo, então
//     re-provar o comportamento interno-de-motor uma segunda vez pelo `App` seria cobertura
//     redundante, não uma propriedade nova capaz de matar mutação; só o FIO de forward
//     ESPECÍFICO do `App` era o que estava desverificado).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

bool read_ppm(const char* path, std::vector<unsigned char>& data, int& w, int& h) {
  FILE* f = fopen(path, "rb");
  if (!f) return false;
  char magic[3] = {0};
  if (fscanf(f, "%2s %d %d %*d%*c", magic, &w, &h) != 3) {
    fclose(f);
    return false;
  }
  data.resize(static_cast<size_t>(w) * h * 3);
  fread(data.data(), 1, data.size(), f);
  fclose(f);
  return true;
}

// EN: Same "ink" oracle as fontface_load_sanity.cpp/fonteng_own_sanity.cpp: max-channel >
//     threshold within a pixel rectangle. PPM is already top-down (App::snapshot() captures
//     FBO 0 in RmlUi's own row order, no glReadPixels/flip needed here -- confirmed by
//     fonteng_own_sanity.cpp's own identical readback, which needs no flip either).
// PT: Mesmo oráculo de "tinta" de fontface_load_sanity.cpp/fonteng_own_sanity.cpp: canal máximo
//     > threshold dentro de um retângulo de pixel. O PPM já é top-down (App::snapshot() captura
//     o FBO 0 na própria ordem de linha do RmlUi, sem precisar de glReadPixels/flip aqui --
//     confirmado pela leitura idêntica de fonteng_own_sanity.cpp, que também não precisa de
//     flip).
long count_ink_rect(const std::vector<unsigned char>& px, int fw, int fh, float rx, float ry,
                    float rw, float rh, int threshold = 100) {
  int x0 = std::max(0, static_cast<int>(rx));
  int y0 = std::max(0, static_cast<int>(ry));
  int x1 = std::min(fw, static_cast<int>(rx + rw) + 1);
  int y1 = std::min(fh, static_cast<int>(ry + rh) + 1);
  long count = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * fw + x) * 3;
      if (px[i] > threshold || px[i + 1] > threshold || px[i + 2] > threshold) ++count;
    }
  }
  return count;
}

} // namespace

int main() {
  int failures = 0;

  glintfx::App app({.title = "app_font_face_sanity", .width = 640, .height = 260});
  if (!app.ok()) {
    // EN: [A3] Same headless-environment escape hatch as fonteng_own_sanity.cpp/app_smoke.cpp
    //     -- no display/GL context available is a SKIP, not a suite failure.
    // PT: [A3] Mesma saída de escape de ambiente headless de fonteng_own_sanity.cpp/
    //     app_smoke.cpp -- sem display/contexto GL disponível é SKIP, não falha de suíte.
    std::puts("SKIP: no display / GL context");
    return 0;
  }

  // ---- [A2] hostile input: path=nullptr must fail closed, no crash --------
  glintfx::FontFaceDesc bad{};
  bad.family = "hostile";
  bad.path = nullptr;
  if (app.load_font_face(bad)) {
    std::fprintf(stderr, "app_font_face_sanity FAIL: [A2] load_font_face(path=nullptr) returned true\n");
    ++failures;
  }

  // ---- [A1] valid registration + render, through App specifically ---------
  glintfx::FontFaceDesc good{};
  good.path = "tests/assets/OpenSans-Regular.ttf";
  good.family = "AppTestFace";

  if (!app.load_font_face(good)) {
    std::fprintf(stderr,
                 "app_font_face_sanity FAIL: [A1] load_font_face(valid, explicit family) "
                 "returned false through App -- forward to Engine::load_font_face broken?\n");
    ++failures;
  } else if (!app.load("tests/app_font_face_scene.rml")) {
    std::fprintf(stderr, "app_font_face_sanity FAIL: [A1] App::load(app_font_face_scene.rml) returned false\n");
    ++failures;
  } else {
    app.set_property("txt", "font-family", "AppTestFace");
    app.set_text("txt", "Hello");

    app.poll_events();
    app.update();
    app.render();
    app.poll_events();
    app.update();
    app.render();

    if (!app.snapshot("app_font_face_sanity_out.ppm")) {
      std::fprintf(stderr, "app_font_face_sanity FAIL: [A1] App::snapshot() returned false\n");
      ++failures;
    } else {
      std::vector<unsigned char> px;
      int w = 0, h = 0;
      if (!read_ppm("app_font_face_sanity_out.ppm", px, w, h) || w <= 0 || h <= 0) {
        std::fprintf(stderr, "app_font_face_sanity FAIL: [A1] could not read app_font_face_sanity_out.ppm\n");
        ++failures;
      } else {
        const glintfx::ElementBox box = app.get_element_box("txt");
        if (!box.found) {
          std::fprintf(stderr, "app_font_face_sanity FAIL: [A1] get_element_box(\"txt\") not found\n");
          ++failures;
        } else {
          constexpr float kMinWidthPx = 6.0f;
          constexpr long kMinInk = 10;
          const long ink = count_ink_rect(px, w, h, box.x, box.y, box.w, box.h);
          if (box.w < kMinWidthPx) {
            std::fprintf(stderr,
                         "app_font_face_sanity FAIL: [A1] rendered span width too small "
                         "(w=%.3f) -- App::load_font_face's font likely did not resolve\n",
                         box.w);
            ++failures;
          } else if (ink < kMinInk) {
            std::fprintf(stderr,
                         "app_font_face_sanity FAIL: [A1] rendered span ink too low (ink=%ld) "
                         "-- App::load_font_face registered the face but it never reached the "
                         "font engine actually used to render this document (broken forward)\n",
                         ink);
            ++failures;
          } else {
            std::printf(
                "app_font_face_sanity [A1] App::load_font_face forwarded correctly and "
                "rendered (w=%.2f ink=%ld)\n",
                box.w, ink);
          }
        }
      }
    }
  }

  if (failures == 0) {
    std::puts("app_font_face_sanity: PASS (App::load_font_face forwards to Engine, hostile input rejected)");
    return 0;
  }
  std::fprintf(stderr, "app_font_face_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
