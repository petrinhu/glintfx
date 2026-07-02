// SPDX-License-Identifier: MPL-2.0
// glintfx — drop-in UI engine + GL3 effects facade.
// Fachada drop-in: motor de UI + efeitos GL3.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <memory>
#include <cstddef>
#include <functional>
// EN: version() moved to its own header (L1.9-VERSEMBED) so embed-only consumers that
//     never include app.hpp still get glintfx::version(). Re-included here (transitively)
//     so existing code that only includes app.hpp keeps compiling unchanged.
// PT: version() foi movida para header próprio (L1.9-VERSEMBED) para que consumidores
//     embed-only, que nunca incluem app.hpp, ainda tenham glintfx::version(). Reincluída
//     aqui (transitivamente) para que código existente que só inclui app.hpp continue
//     compilando sem mudanças.
#include <glintfx/version.hpp>
namespace glintfx {

// EN: Configuration for App construction. Zero-initialize safe; defaults are sane.
// PT: Configuração para construção do App. Zero-init seguro; defaults razoáveis.
struct AppConfig {
  const char* title = "glintfx";
  int width  = 1280;
  int height = 720;
  // EN: Initial density-independent pixel ratio. 1 dp = dp_ratio physical pixels.
  //     Matches UiLayerConfig::dp_ratio; see set_dp_ratio() for runtime update.
  // PT: Density-independent pixel ratio inicial. 1 dp = dp_ratio pixels físicos.
  //     Corresponde a UiLayerConfig::dp_ratio; ver set_dp_ratio() para update em runtime.
  float dp_ratio = 1.0f;
};

// EN: RAII application facade. Owns the window, renderer, and UI bootstrap.
//     Move-only. No third-party engine or graphics types appear in this header.
//
//     GLOBAL STATE (N3): glintfx initialises process-global state (window backend + UI library).
//     Only ONE App instance per process is supported. Creating a second instance while
//     the first is alive, or after it has been destroyed, results in undefined behaviour.
//
//     MOVED-FROM STATE (L1.10-APIDOC): after `App b = std::move(a);` (or `b = std::move(a);`),
//     `a` is left in a moved-from state. Calling ANY method on `a` other than ok() or the
//     destructor is undefined behaviour — same convention as std::unique_ptr. ok() on a
//     moved-from App returns false (impl_ is null), which is the only state query that is
//     safe to make; destruction of a moved-from App is always safe (no-op, impl_ is null).
//
// PT: Fachada RAII da aplicação. Possui janela, renderer e bootstrap de UI.
//     Move-only. Nenhum tipo de engine ou gráficos de terceiros aparece neste header.
//
//     ESTADO GLOBAL (N3): glintfx inicializa estado global de processo (backend de janela + lib de UI).
//     Apenas UMA instância de App por processo é suportada. Criar uma segunda instância
//     enquanto a primeira está viva, ou após ser destruída, resulta em comportamento indefinido.
//
//     ESTADO MOVIDO-DE (L1.10-APIDOC): após `App b = std::move(a);` (ou `b = std::move(a);`),
//     `a` fica em estado movido-de. Chamar QUALQUER método em `a` além de ok() ou o destrutor
//     é comportamento indefinido — mesma convenção do std::unique_ptr. ok() num App movido-de
//     retorna false (impl_ é nulo), sendo a única consulta de estado segura; destruir um App
//     movido-de é sempre seguro (no-op, impl_ é nulo).
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
  // EN: Move leaves the source in a moved-from state — see the class-level comment above
  //     for the exact contract (only ok() and ~App() remain valid on the source afterwards).
  // PT: O move deixa a origem em estado movido-de — ver o comentário de nível de classe
  //     acima para o contrato exato (só ok() e ~App() permanecem válidos na origem depois).
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

  // EN: Load an .rml document and show it. Returns true on success.
  // PT: Carrega um documento .rml e exibe. Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Update the density-independent pixel ratio at runtime (parity with UiLayer).
  //     Triggers a layout re-pass when changed.
  // PT: Atualiza o density-independent pixel ratio em runtime (paridade com UiLayer).
  //     Dispara re-layout quando alterado.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL for asset resolution (parity with UiLayer).
  //     Relative asset paths are prefixed with base_url/. Call before load().
  // PT: Sobrepõe o base URL para resolução de assets (paridade com UiLayer).
  //     Caminhos relativos de assets são prefixados com base_url/. Chame antes de load().
  void set_asset_base_url(const char* url);

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

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none reached before the document root).
  //     No ordering constraint versus load(): safe to call before or after it. Parity with
  //     UiLayer::set_click_callback (same signature).
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum for encontrado até a raiz do
  //     documento). Sem restrição de ordem vs. load(): seguro chamar antes ou depois. Paridade
  //     com UiLayer::set_click_callback (mesma assinatura).
  void set_click_callback(std::function<void(const char* element_id)> cb);

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1) — parity with UiLayer. Call order: create_data_model
  //     -> bind_* -> load() -> set_*(). Engine enforces the ordering constraint
  //     (bind_* after load() returns false). No engine-specific types in this header.
  // PT: API de data-model (T1) — paridade com UiLayer. Ordem de chamada:
  //     create_data_model -> bind_* -> load() -> set_*(). Engine enforça a
  //     restrição de ordem (bind_* após load() retorna false). API usa tipos C simples.
  // -------------------------------------------------------------------------

  // EN: Create a data model. Call before bind_* and before load().
  // PT: Cria um data model. Chamar antes de bind_* e antes de load().
  bool create_data_model(const char* name);

  // EN: Bind a numeric (double) cell with an optional initial value.
  // PT: Liga uma célula numérica (double) com valor inicial opcional.
  bool bind_number(const char* key, double initial = 0.0);

  // EN: Bind a string cell.
  // PT: Liga uma célula de string.
  bool bind_string(const char* key, const char* initial = "");

  // EN: Bind a boolean cell.
  // PT: Liga uma célula booleana.
  bool bind_bool  (const char* key, bool initial = false);

  // EN: Bind a string-list cell (for data-for iteration in RML).
  // PT: Liga uma célula de lista de strings (para iteração data-for no RML).
  bool bind_list  (const char* key);

  // EN: Update a numeric cell after load. No-op when key is unknown.
  //     The change is reflected on the next update() call.
  // PT: Atualiza uma célula numérica após load. No-op quando a chave é desconhecida.
  //     A mudança é refletida na próxima chamada a update().
  void set_number(const char* key, double value);

  // EN: Update a string cell after load.
  // PT: Atualiza uma célula de string após load.
  void set_string(const char* key, const char* value);

  // EN: Update a boolean cell after load.
  // PT: Atualiza uma célula booleana após load.
  void set_bool  (const char* key, bool value);

  // EN: Replace the entire string list and dirty the variable.
  //     items[0..count-1] are copied; caller retains ownership.
  // PT: Substitui a lista de strings inteira e marca a variável suja.
  //     items[0..count-1] são copiados; o chamador retém a propriedade.
  void set_list  (const char* key, const char* const* items, std::size_t count);

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
