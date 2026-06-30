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

private:
  struct Impl;
  Impl* impl_;
};

} // namespace glintfx
