// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap — wire SystemInterface (GLFW) + RenderInterface into RmlUi, create Context.
//     No RmlUi or GL types leak through this header (pImpl pattern).
// PT: Bootstrap — conecta SystemInterface (GLFW) + RenderInterface ao RmlUi, cria Context.
//     Nenhum tipo RmlUi ou GL vaza por este header (padrão pImpl).
#pragma once

// EN: Forward-declare only — callers must not depend on RmlUi headers.
// PT: Só forward-declaration — callers não devem depender dos headers do RmlUi.
namespace Rml { class Context; }

namespace glintfx {

class WindowGlfw;
class RenderGl3;

// EN: RAII bootstrap for RmlUi. One instance per application lifetime; not copyable.
// PT: Bootstrap RAII para RmlUi. Uma instância por lifetime da aplicação; não copiável.
class Bootstrap {
public:
  Bootstrap() = default;
  ~Bootstrap();
  Bootstrap(const Bootstrap&) = delete;
  Bootstrap& operator=(const Bootstrap&) = delete;

  // EN: Set up SystemInterface_GLFW and RenderInterface, call Rml::Initialise(), and
  //     create a context named "main" with dimensions w×h.
  //     Returns true on success. On failure the object is left in a safe state for
  //     shutdown() or destruction.
  // PT: Configura SystemInterface_GLFW e RenderInterface, chama Rml::Initialise() e
  //     cria contexto "main" com dimensões w×h.
  //     Retorna true em caso de sucesso. Em falha o objeto fica em estado seguro para
  //     shutdown() ou destruição.
  bool init(WindowGlfw& win, RenderGl3& render, int w, int h);

  // EN: Load a document from rml_path and call Show(). Returns true on success.
  // PT: Carrega documento de rml_path e chama Show(). Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Returns the active Rml::Context (valid after successful init()), or nullptr.
  // PT: Retorna o Rml::Context ativo (válido após init() com sucesso), ou nullptr.
  Rml::Context* context();

  // EN: Shutdown RmlUi and release all resources. Safe to call multiple times.
  // PT: Encerra o RmlUi e libera todos os recursos. Seguro chamar múltiplas vezes.
  void shutdown();

private:
  struct Impl;        // EN: defined in bootstrap.cpp; hides RmlUi types.
                      // PT: definido em bootstrap.cpp; oculta tipos RmlUi.
  Impl* impl_ = nullptr;
};

} // namespace glintfx
