// SPDX-License-Identifier: MPL-2.0
// EN: Dev/QA tool — render the showcase once and snapshot it to a PPM, for real-GPU
//     validation/comparison (Intel vs NVIDIA). Uses ONLY the public glintfx facade.
//     Usage: glintfx_capture [rml_path] [out_ppm]
// PT: Ferramenta dev/QA — renderiza o showcase uma vez e salva em PPM, para validação/
//     comparação em GPU real (Intel vs NVIDIA). Usa SÓ a fachada pública glintfx.
//     Uso: glintfx_capture [caminho_rml] [saida_ppm]
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>

int main(int argc, char** argv) {
  const char* rml = argc > 1 ? argv[1] : "showcase.rml";
  const char* out = argc > 2 ? argv[2] : "capture.ppm";
  glintfx::AppConfig cfg;
  cfg.title = "glintfx capture";
  cfg.width = 900;
  cfg.height = 600;
  glintfx::App app(cfg);
  app.load(rml);
  // EN: warm up a few frames so layout and effects settle. PT: aquece alguns frames.
  for (int i = 0; i < 4; ++i) { app.poll_events(); app.update(); app.render(); }
  return app.snapshot(out) ? 0 : 1;
}
