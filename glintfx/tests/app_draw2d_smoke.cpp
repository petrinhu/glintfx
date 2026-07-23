// SPDX-License-Identifier: MPL-2.0
// EN: app_draw2d_smoke (D2D-1B, GLFW=ON leg) -- proves `glintfx::Draw2d` works from inside
//     `App::set_frame_callback` (D6: standalone composition recipe -- draw calls go inside the
//     frame hook, so the scene composes UNDER the UI, the pre-existing v0.19.0 contract,
//     app.hpp's own doc-comment). Also pins that `Draw2d::init()` called AFTER an `App` is
//     already constructed works (D2: `glx_gl_load()` is idempotent-safe even though `App`'s own
//     ctor already loaded the same table for the engine's internal renderer).
//
//     Oracle (same PPM-snapshot idiom as app_frame_callback_smoke.cpp): a white opaque UI box at
//     (0,0)-(100,100) and a sprite drawn INSIDE the frame hook at dst=(50,50,120,120) -- they
//     OVERLAP in (50,50)-(100,100). Three sample points:
//       - UI-only region (25,25): must be the UI's white.
//       - sprite-only region (140,140): must be the sprite's own solid colour.
//       - the OVERLAP region (75,75): must ALSO be white -- proving the sprite pass ran BEFORE
//         Context::Render() (Engine::render_standalone's documented insertion point) and the
//         UI's opaque quad painted over it, i.e. sprites compose UNDER the UI in standalone mode.
// PT: app_draw2d_smoke (D2D-1B, perna GLFW=ON) -- prova que `glintfx::Draw2d` funciona de dentro
//     de `App::set_frame_callback` (D6: receita de composição standalone -- as chamadas de
//     desenho vão dentro do frame hook, então a cena compõe SOB a UI, o contrato pré-existente
//     da v0.19.0, doc-comment próprio do app.hpp). Também fixa que `Draw2d::init()` chamado
//     DEPOIS de um `App` já construído funciona (D2: `glx_gl_load()` é seguro de chamar de forma
//     idempotente mesmo que o próprio ctor do `App` já tenha carregado a mesma tabela para o
//     renderer interno do engine).
//
//     Oráculo (mesmo idioma de snapshot PPM de app_frame_callback_smoke.cpp): uma caixa de UI
//     branca opaca em (0,0)-(100,100) e um sprite desenhado DENTRO do frame hook em
//     dst=(50,50,120,120) -- eles SE SOBREPÕEM em (50,50)-(100,100). Três pontos de amostra:
//       - região só-UI (25,25): precisa ser o branco da UI.
//       - região só-sprite (140,140): precisa ser a própria cor sólida do sprite.
//       - a região de SOBREPOSIÇÃO (75,75): precisa TAMBÉM ser branca -- provando que o passe de
//         sprite rodou ANTES de Context::Render() (o ponto de inserção documentado de
//         Engine::render_standalone) e o quad opaco da UI pintou por cima, ou seja, sprites
//         compõem SOB a UI no modo standalone.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using glintfx::App;
using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::Texture2d;

namespace {

std::vector<unsigned char> build_tga_solid(int w, int h, unsigned char r, unsigned char g,
                                            unsigned char b) {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0); f.push_back(0); f.push_back(2);
  push16(0); push16(0); f.push_back(0);
  push16(0); push16(0);
  push16(w); push16(h);
  f.push_back(24);
  f.push_back(0x20);
  for (int i = 0; i < w * h; ++i) { f.push_back(b); f.push_back(g); f.push_back(r); }
  return f;
}

bool write_file(const fs::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return f.good();
}

bool write_ui_box_rml(const fs::path& path, int box_size) {
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "<rml>\n<head>\n<style>\n"
       "body { margin: 0; padding: 0; background-color: #0a1428; }\n"
       "#box { position: absolute; left: 0px; top: 0px; width: "
    << box_size << "px; height: " << box_size
    << "px; background-color: white; }\n"
       "</style>\n<title>app_draw2d_smoke probe</title>\n</head>\n<body>\n<div id=\"box\"></div>\n</body>\n</rml>\n";
  return f.good();
}

bool sample_ppm_pixel(const char* path, int w, int h, int x, int y, int& r, int& g, int& b) {
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  char magic[3] = {0};
  int fw = 0, fh = 0, maxval = 0;
  if (std::fscanf(f, "%2s %d %d %d", magic, &fw, &fh, &maxval) != 4 || magic[0] != 'P' ||
      magic[1] != '6' || fw != w || fh != h) {
    std::fclose(f);
    return false;
  }
  std::fgetc(f); // single whitespace byte after the header, P6 format.
  // EN: App::snapshot()'s PPM is already top-left-origin row order (see app.cpp's own
  //     snapshot() doc-comment) -- same convention app_frame_callback_smoke.cpp relies on.
  // PT: O PPM de App::snapshot() já está em ordem de linha origem-superior-esquerda (ver o
  //     próprio doc-comment de snapshot() em app.cpp) -- mesma convenção que
  //     app_frame_callback_smoke.cpp usa.
  const long offset = static_cast<long>(y) * w * 3 + static_cast<long>(x) * 3;
  if (std::fseek(f, offset, SEEK_CUR) != 0) { std::fclose(f); return false; }
  unsigned char rgb[3];
  const bool ok = std::fread(rgb, 1, 3, f) == 3;
  std::fclose(f);
  if (!ok) return false;
  r = rgb[0]; g = rgb[1]; b = rgb[2];
  return true;
}

bool near_channel(int got, int want, int tol) { return std::abs(got - want) <= tol; }

} // namespace

int main() {
  constexpr int W = 200, H = 200;
  constexpr int kTol = 12;

  App app({.title = "app_draw2d_smoke", .width = W, .height = H});
  if (!app.ok()) {
    std::puts("app_draw2d_smoke FAIL: app ok() false");
    return 1;
  }

  char tmpl[] = "/var/tmp/glintfx_app_draw2d_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("app_draw2d_smoke FAIL: mkdtemp failed");
    return 2;
  }
  const fs::path dir(dir_c);
  const fs::path p_sprite = dir / "sprite.tga";
  const fs::path p_rml = dir / "box.rml";
  write_file(p_sprite, build_tga_solid(8, 8, 200, 100, 50));
  write_ui_box_rml(p_rml, 100);

  if (!app.load(p_rml.c_str())) {
    std::puts("app_draw2d_smoke FAIL: could not load UI probe document");
    return 3;
  }

  // D2: init() called AFTER App's own construction (the engine already loaded glx_gl_load for
  // itself) -- must still succeed, idempotently.
  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("app_draw2d_smoke FAIL: Draw2d::init() after App construction failed");
    return 4;
  }
  Texture2d tex = d2d.load_texture(p_sprite.c_str());
  if (!tex.ok()) {
    std::puts("app_draw2d_smoke FAIL: sprite texture failed to load");
    return 5;
  }

  app.set_frame_callback([&](float) {
    int w = 0, h = 0;
    app.get_window_size(w, h);
    d2d.begin(w, h);
    d2d.draw_sprite(tex, RectF{50, 50, 120, 120});
    d2d.end();
  });

  // EN: snapshot() does NOT call update() itself (see app.cpp's own snapshot() doc-comment) --
  // an explicit update() first is required so RmlUi has actually run layout on the loaded
  // document before the frame this snapshot captures (app_frame_callback_smoke.cpp's own pixel-
  // proof leg relies on an update() cycle having already run for the same reason).
  // PT: snapshot() NÃO chama update() por conta própria (ver o próprio doc-comment de snapshot()
  // em app.cpp) -- um update() explícito antes é necessário pra que o RmlUi de fato tenha rodado
  // layout no documento carregado antes do frame que este snapshot captura (a própria perna de
  // prova por pixel de app_frame_callback_smoke.cpp depende de um ciclo update() já ter rodado
  // pelo mesmo motivo).
  app.update();

  const char* kSnapshotPath = "app_draw2d_smoke.ppm";
  if (!app.snapshot(kSnapshotPath)) {
    std::puts("app_draw2d_smoke FAIL: snapshot() returned false");
    return 6;
  }
  if (!app.ok()) {
    std::puts("app_draw2d_smoke FAIL: app.ok() went false after the frame");
    return 7;
  }

  int failures = 0;
  auto check_pixel = [&](const char* label, int x, int y, int want_r, int want_g, int want_b) {
    int r = 0, g = 0, b = 0;
    if (!sample_ppm_pixel(kSnapshotPath, W, H, x, y, r, g, b)) {
      std::printf("FAIL: %s: could not sample pixel (%d,%d)\n", label, x, y);
      ++failures;
      return;
    }
    std::printf("app_draw2d_smoke [%s]: (%d,%d) = (%d,%d,%d), expect ~(%d,%d,%d)\n", label, x, y,
                r, g, b, want_r, want_g, want_b);
    if (!near_channel(r, want_r, kTol) || !near_channel(g, want_g, kTol) ||
        !near_channel(b, want_b, kTol)) {
      std::printf("FAIL: %s: pixel out of tolerance\n", label);
      ++failures;
    }
  };

  check_pixel("ui-only", 25, 25, 255, 255, 255);
  check_pixel("sprite-only", 140, 140, 200, 100, 50);
  // Overlap: the sprite pass ran inside the frame hook, BEFORE Context::Render() -- the UI's
  // opaque box must paint over it here (D6: sprites compose UNDER the UI in standalone mode).
  check_pixel("overlap-ui-wins", 75, 75, 255, 255, 255);

  std::error_code ec;
  fs::remove_all(dir, ec);

  if (failures == 0) {
    std::puts("app_draw2d_smoke: PASS");
    return 0;
  }
  std::printf("app_draw2d_smoke: %d check(s) FAILED\n", failures);
  return failures;
}
