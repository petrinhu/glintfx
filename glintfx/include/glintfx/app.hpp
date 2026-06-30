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
//
//     GLOBAL STATE (N3): glintfx initialises process-global libraries (GLFW, RmlUi).
//     Only ONE App instance per process is supported. Creating a second instance while
//     the first is alive, or after it has been destroyed, results in undefined behaviour.
//
// PT: Fachada RAII da aplicação. Possui janela, renderer e bootstrap de UI.
//     Move-only. Nenhum tipo de engine ou gráficos de terceiros aparece neste header.
//
//     ESTADO GLOBAL (N3): glintfx inicializa bibliotecas globais de processo (GLFW, RmlUi).
//     Apenas UMA instância de App por processo é suportada. Criar uma segunda instância
//     enquanto a primeira está viva, ou após ser destruída, resulta em comportamento indefinido.
class App {
public:
  // EN: Constructs the App: opens a window and initialises the GL context and UI engine.
  //     Construction can fail silently (e.g. no display, no GL context available).
  //     On failure (N2): ok() returns false; running() also returns false; load() is a no-op;
  //     render() and run() do nothing. Check ok() once after construction to distinguish an
  //     initialization failure from a window that was closed after a successful start.
  //     run() is safe to call regardless — it returns immediately when !running().
  // PT: Constrói o App: abre a janela e inicializa o contexto GL e o motor de UI.
  //     A construção pode falhar silenciosamente (ex.: sem display, sem contexto GL disponível).
  //     Em falha (N2): ok() retorna false; running() também retorna false; load() é no-op;
  //     render() e run() não fazem nada. Verifique ok() uma vez após a construção para distinguir
  //     falha de inicialização de uma janela fechada após início bem-sucedido.
  //     run() é seguro de chamar independentemente — retorna imediatamente quando !running().
  explicit App(AppConfig cfg = {});
  ~App();
  App(App&&) noexcept;
  App& operator=(App&&) noexcept;
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // EN: Returns true if construction succeeded: window, GL context, and UI engine are all live.
  //     Use ok() to distinguish "initialization failed" from "window closed":
  //     running() returns false in both cases, conflating them. Check ok() once right after
  //     construction; it never changes after that point.
  // PT: Retorna true se a construção teve sucesso: janela, contexto GL e motor de UI estão todos vivos.
  //     Use ok() para distinguir "falha de inicialização" de "janela fechada":
  //     running() retorna false nos dois casos, misturando-os. Verifique ok() uma vez logo após
  //     a construção; ele nunca muda depois desse ponto.
  bool ok() const noexcept;

  // EN: Load an .rml document and show it.
  // PT: Carrega um documento .rml e exibe.
  void load(const char* rml_path);

  // EN: Returns false if the window was closed or initialization failed.
  //     To distinguish the two cases check ok() once after construction.
  // PT: Retorna false se a janela foi fechada ou se a inicialização falhou.
  //     Para distinguir os dois casos, verifique ok() uma vez após a construção.
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

  // EN: Render one frame and save the result to a PPM file before swapping buffers.
  //     This captures the composited image from the window framebuffer (FBO 0) immediately
  //     after EndFrame composites the render layer, ensuring correct pixel data even on
  //     platforms where the back buffer content is undefined after glfwSwapBuffers.
  //     Returns true on success.
  // PT: Renderiza um frame e salva o resultado em arquivo PPM antes de trocar buffers.
  //     Captura a imagem composta do framebuffer da janela (FBO 0) imediatamente após
  //     o EndFrame compositar o render layer, garantindo dados corretos mesmo em plataformas
  //     onde o back buffer fica indefinido após glfwSwapBuffers.
  //     Retorna true em caso de sucesso.
  bool snapshot(const char* ppm_path);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
