// SPDX-License-Identifier: MPL-2.0
// EN: Verifies UiLayer::get_element_box(): (A) found element returns the exact RCSS-authored
//     border-box geometry (left:20 top:15 60x40 -- absolute positioning, no margin/border/
//     padding in this scene, so border-box == the raw left/top/width/height); (B) unknown id
//     returns found=false with all-zero fields; (C) no document loaded yet returns found=false;
//     (D) id="" (AUD-TEC-5) returns found=false -- empty string must fail-high like null, not
//     silently fall through to a GetElementById("") lookup.
// PT: Verifica UiLayer::get_element_box(): (A) elemento encontrado retorna exatamente a
//     geometria border-box autorada no RCSS (left:20 top:15 60x40 -- posicionamento absoluto,
//     sem margem/borda/padding nesta cena, então border-box == left/top/width/height crus);
//     (B) id desconhecido retorna found=false com campos todos zero; (C) nenhum documento
//     carregado ainda retorna found=false; (D) id="" (AUD-TEC-5) retorna found=false -- string
//     vazia deve falhar fail-high como nulo, não cair silenciosamente numa busca
//     GetElementById("").
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cmath>

static bool approx(float a, float b, float tol = 0.5f) { return std::fabs(a - b) <= tol; }

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("elbox_host", 200, 150)) { std::puts("FAIL: host create"); return 1; }

  glintfx::UiLayer ui({ .logical_width = 200, .logical_height = 150, .load_gl = true });
  if (!ui.ok()) { std::puts("FAIL: ui attach"); return 2; }

  // (C) no document loaded yet.
  auto pre = ui.get_element_box("probe");
  if (pre.found) { std::puts("FAIL: found=true before any load()"); return 3; }

  if (!ui.load("element_box_scene.rml")) { std::puts("FAIL: load"); return 4; }
  ui.update(); ui.render();

  // (A) known id.
  auto box = ui.get_element_box("probe");
  if (!box.found) { std::puts("FAIL: found=false for existing id"); return 5; }
  if (!approx(box.x, 20.f) || !approx(box.y, 15.f) ||
      !approx(box.w, 60.f) || !approx(box.h, 40.f)) {
    std::fprintf(stderr, "FAIL: box=(%.1f,%.1f,%.1f,%.1f) expected ~(20,15,60,40)\n",
                 box.x, box.y, box.w, box.h);
    return 6;
  }

  // (B) unknown id.
  auto missing = ui.get_element_box("does_not_exist");
  if (missing.found || missing.x != 0.f || missing.w != 0.f) {
    std::puts("FAIL: unknown id did not return the zeroed default");
    return 7;
  }

  // (D) id="" (AUD-TEC-5) -- must fail-high like null, not fall through to GetElementById("").
  auto empty_id = ui.get_element_box("");
  if (empty_id.found || empty_id.x != 0.f || empty_id.w != 0.f) {
    std::puts("FAIL: id=\"\" did not return the zeroed found=false default");
    return 8;
  }

  std::puts("element_box_sanity: PASS");
  return 0;
}
