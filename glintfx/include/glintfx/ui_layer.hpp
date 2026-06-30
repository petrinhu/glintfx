// SPDX-License-Identifier: MPL-2.0
// EN: Embed/guest facade — attaches the UI+effects engine to a GL context the HOST owns.
//     Does not create a window; render() is compose-only (no clear, no swap).
// PT: Fachada embed/guest — anexa o motor de UI+efeitos a um contexto GL do HOST.
//     Não cria janela; render() é compose-only (sem clear, sem swap).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <memory>
#include <glintfx/ui_event.hpp>

namespace glintfx {

// EN: Configuration for UiLayer — specifies logical viewport dimensions and GL loader policy.
//     Declared at namespace scope so it is a complete type when used as a default argument
//     in the UiLayer constructor (inner-class default-args hit a C++ language restriction).
// PT: Configuração para UiLayer — especifica dimensões lógicas do viewport e política do GL loader.
//     Declarada em namespace scope para ser tipo completo ao ser usada como argumento padrão
//     no construtor do UiLayer (default-args de inner-class atingem uma restrição da linguagem C++).
struct UiLayerConfig {
  int logical_width  = 1280;
  int logical_height = 720;
  bool load_gl       = true;
};

class UiLayer {
public:
  // EN: Inner alias so callers can write UiLayer::Config{...} — matches the spec interface.
  // PT: Alias interno para que chamadores possam escrever UiLayer::Config{...} — segue a spec.
  using Config = UiLayerConfig;

  // EN: Requires a CURRENT GL context owned by the host.
  //     GL function pointers are loaded (idempotent per-process) when cfg.load_gl is true.
  // PT: Exige contexto GL CORRENTE do host.
  //     Ponteiros de função GL são carregados (idempotente por processo) quando cfg.load_gl for true.
  explicit UiLayer(Config cfg = Config{});
  ~UiLayer();

  UiLayer(UiLayer&&) noexcept;
  UiLayer& operator=(UiLayer&&) noexcept;
  UiLayer(const UiLayer&)            = delete;
  UiLayer& operator=(const UiLayer&) = delete;

  // EN: True when attach + engine init succeeded.
  // PT: True quando attach + init do motor foram bem-sucedidos.
  bool ok() const noexcept;

  // EN: Load a document from rml_path and call Show(). Requires ok().
  // PT: Carrega documento de rml_path e chama Show(). Requer ok().
  void load(const char* rml_path);

  // EN: Notify of a viewport resize (real target pixels).
  // PT: Notifica redimensionamento de viewport (pixels reais do alvo).
  void set_viewport(int w, int h);

  // EN: Advance the UI context by one step (call once per frame before render()).
  // PT: Avança o contexto de UI um passo (chamar uma vez por frame antes de render()).
  void update();

  // EN: Compose-only over the current FBO — no glClear, no swap, GL state guarded (T3).
  // PT: Compose-only sobre o FBO corrente — sem glClear, sem swap, estado GL protegido (T3).
  void render();

  // EN: Inject a host input event — translated to engine input calls internally (T4).
  // PT: Injeta evento de entrada do host — traduzido para chamadas de input do motor internamente (T4).
  void process_event(const UiEvent& ev);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
