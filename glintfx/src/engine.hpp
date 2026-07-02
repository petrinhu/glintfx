// SPDX-License-Identifier: MPL-2.0
// EN: Engine — reusable RmlUi+GL3 core decoupled from any window/input backend.
//     Owns a RenderGl3 + Bootstrap pair. Two frame paths:
//       render_standalone — clear + UI + alpha-fix on FBO0 (used by App).
//       render_compose    — compose-only, no clear/no swap, with GL state guard (T3, UiLayer).
//     Callers supply a Rml::SystemInterface* (owned by the caller, not by Engine).
// PT: Engine — núcleo RmlUi+GL3 reusável desacoplado de qualquer backend de janela/input.
//     Possui um par RenderGl3 + Bootstrap. Dois caminhos de frame:
//       render_standalone — clear + UI + alpha-fix no FBO0 (usado pelo App).
//       render_compose    — compose-only, sem clear/sem swap, com guard de estado GL (T3, UiLayer).
//     Chamadores fornecem um Rml::SystemInterface* (dono do chamador, não do Engine).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: Forward-declare only — no RmlUi/GL types visible to callers.
// PT: Só forward-declaration — nenhum tipo RmlUi/GL visível aos chamadores.
namespace Rml { class Context; class SystemInterface; }
#include <cstddef>
#include <functional>

namespace glintfx {

class Engine {
public:
  Engine();
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  // EN: Initialise the RmlUi+GL3 subsystem against the CURRENT GL context.
  //     `system` lifetime is owned by the caller; must stay alive until ~Engine().
  //     Returns true on success. Safe to call only once per instance.
  // PT: Inicializa o subsistema RmlUi+GL3 contra o contexto GL CORRENTE.
  //     O lifetime de `system` pertence ao chamador; deve sobreviver até ~Engine().
  //     Retorna true em caso de sucesso. Seguro chamar apenas uma vez por instância.
  bool attach(Rml::SystemInterface* system, int w, int h);

  // EN: True after a successful attach().
  // PT: True após um attach() com sucesso.
  bool ok() const noexcept;

  // EN: Load a document from rml_path and call Show(). Requires ok().
  // PT: Carrega documento de rml_path e chama Show(). Requer ok().
  bool load(const char* rml_path);

  // EN: Notify RmlUi of a viewport resize (logical dimensions).
  // PT: Notifica o RmlUi de um redimensionamento de viewport (dimensões lógicas).
  void set_viewport(int w, int h);

  // EN: Set the density-independent pixel ratio on the active context.
  //     1 dp = ratio physical pixels. Call after a successful attach().
  //     No-op when not ok(). Triggers a full layout re-pass if ratio changes.
  // PT: Define o density-independent pixel ratio no contexto ativo.
  //     1 dp = ratio pixels físicos. Chamar após attach() com sucesso.
  //     No-op quando não ok(). Dispara re-layout completo se ratio mudar.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL used to resolve relative asset paths (fonts, images, RCSS).
  //     Delegates to Bootstrap's FileInterface; safe to call at any time after attach().
  //     Pass nullptr or "" to clear. No-op when not ok().
  // PT: Sobrepõe o base URL usado para resolver caminhos relativos de assets (fontes, imagens, RCSS).
  //     Delega ao FileInterface do Bootstrap; seguro chamar a qualquer momento após attach().
  //     Passe nullptr ou "" para limpar. No-op quando não ok().
  void set_asset_base_url(const char* url);

  // EN: Advance the RmlUi context by one step (call once per frame before render_*).
  // PT: Avança o contexto RmlUi um passo (chamar uma vez por frame antes de render_*).
  void update();

  // EN: Standalone frame path — clear + render UI + alpha-fix on FBO0.
  //     Used by App (owns the window). Does NOT swap buffers.
  // PT: Caminho de frame standalone — clear + render UI + alpha-fix no FBO0.
  //     Usado pelo App (dono da janela). NÃO faz swap de buffers.
  void render_standalone(int w, int h);

  // EN: Compose-only frame path — no glClear, no swap, GL state guarded (T3).
  //     Implemented in T3. In T1 this is a documented no-op stub.
  // PT: Caminho compose-only — sem glClear, sem swap, estado GL protegido (T3).
  //     Implementado na T3. Na T1 este é um stub no-op documentado.
  void render_compose(int w, int h);

  // EN: Returns the active Rml::Context, or nullptr if not ok().
  // PT: Retorna o Rml::Context ativo, ou nullptr se não ok().
  Rml::Context* context();

  // EN: Register a click callback -- forwards to Bootstrap::set_click_callback (F1, v0.2.5).
  //     See Bootstrap::set_click_callback for the full ordering/lifetime contract.
  // PT: Registra um callback de clique -- encaminha a Bootstrap::set_click_callback (F1,
  //     v0.2.5). Ver Bootstrap::set_click_callback para o contrato completo de ordem/lifetime.
  void set_click_callback(std::function<void(const char*)> cb);

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1). Call order: create_data_model -> bind_* -> load -> set_*.
  //     Binds are registered against the RmlUi context BEFORE LoadDocument so that
  //     data-views are compiled with the correct variable map.
  //     Guards: create_data_model / bind_* return false after load() is called (too late).
  //             set_* are no-ops when the key was never bound (safe any time after load).
  // PT: API de data-model (T1). Ordem: create_data_model -> bind_* -> load -> set_*.
  //     Binds são registrados no contexto RmlUi ANTES do LoadDocument para que
  //     as data-views sejam compiladas com o mapa de variáveis correto.
  //     Guards: create_data_model / bind_* retornam false após load() (tarde demais).
  //             set_* são no-op quando a chave nunca foi ligada (seguro após load).
  // -------------------------------------------------------------------------

  // EN: Create a data model; must be called before bind_* and before load().
  // PT: Cria um data model; deve ser chamado antes de bind_* e antes de load().
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
  // EN: Bind a string-list cell (for data-for iteration).
  // PT: Liga uma célula de lista de strings (para iteração data-for).
  bool bind_list  (const char* key);

  // EN: Mutate a bound cell and dirty the variable (reflected on next update()).
  // PT: Muta uma célula ligada e marca a variável suja (refletido no próximo update()).
  void set_number(const char* key, double v);
  void set_string(const char* key, const char* v);
  void set_bool  (const char* key, bool v);
  void set_list  (const char* key, const char* const* items, std::size_t n);

private:
  struct Impl;
  Impl* impl_;
};

} // namespace glintfx
