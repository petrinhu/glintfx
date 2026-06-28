// SPDX-License-Identifier: MPL-2.0
// glintfx — drop-in UI engine + GL3 effects facade.
// Fachada drop-in: motor de UI + efeitos GL3.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <memory>
namespace glintfx {

// EN: Returns the library version string.
// PT: Retorna a string de versão da lib.
const char* version();

// EN: Configuration for App construction. Zero-initialize safe; defaults are sane.
// PT: Configuração para construção do App. Zero-init seguro; defaults razoáveis.
struct AppConfig {
  const char* title = "glintfx";
  int width  = 1280;
  int height = 720;
};

// EN: RAII application facade. Owns the window, renderer, and UI bootstrap.
//     Move-only. No third-party engine or graphics types appear in this header.
// PT: Fachada RAII da aplicação. Possui janela, renderer e bootstrap de UI.
//     Move-only. Nenhum tipo de engine ou gráficos de terceiros aparece neste header.
class App {
public:
  explicit App(AppConfig cfg = {});
  ~App();
  App(App&&) noexcept;
  App& operator=(App&&) noexcept;
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // EN: Load an .rml document and show it.
  // PT: Carrega um documento .rml e exibe.
  void load(const char* rml_path);

  // EN: Returns false if the window was closed or initialization failed.
  // PT: Retorna false se a janela foi fechada ou se a inicialização falhou.
  bool running() const;

  // EN: Process pending window/input events (non-blocking).
  // PT: Processa eventos de janela/entrada pendentes (sem bloqueio).
  void poll_events();

  // EN: Advance the UI context by one tick.
  // PT: Avança o contexto de UI por um tick.
  void update();

  // EN: Render one frame (begin_frame → UI render pass → end_frame → swap).
  // PT: Renderiza um frame (begin_frame → passe de render de UI → end_frame → swap).
  void render();

  // EN: Convenience loop: poll + update + render until !running().
  // PT: Laço de conveniência: poll + update + render até !running().
  void run();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
