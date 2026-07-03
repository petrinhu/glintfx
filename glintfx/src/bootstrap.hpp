// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap — wire SystemInterface + RenderInterface into RmlUi, create Context.
//     No RmlUi or GL types leak through this header (pImpl pattern).
//     The caller is the OWNER of the SystemInterface lifetime (Bootstrap does not delete it).
// PT: Bootstrap — conecta SystemInterface + RenderInterface ao RmlUi, cria Context.
//     Nenhum tipo RmlUi ou GL vaza por este header (padrão pImpl).
//     O chamador é DONO do lifetime do SystemInterface (Bootstrap não o deleta).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: Forward-declare only — callers must not depend on RmlUi headers.
// PT: Só forward-declaration — callers não devem depender dos headers do RmlUi.
namespace Rml { class Context; class SystemInterface; }

#include <functional>

namespace glintfx {

class RenderGl3;

// EN: RAII bootstrap for RmlUi. One instance per application lifetime; not copyable.
//     The caller supplies a Rml::SystemInterface* (does not have to be GLFW-based).
// PT: Bootstrap RAII para RmlUi. Uma instância por lifetime da aplicação; não copiável.
//     O chamador fornece um Rml::SystemInterface* (não precisa ser baseado em GLFW).
class Bootstrap {
public:
  Bootstrap() = default;
  ~Bootstrap();
  Bootstrap(const Bootstrap&) = delete;
  Bootstrap& operator=(const Bootstrap&) = delete;

  // EN: Set up RmlUi system+render interfaces, call Rml::Initialise(), and create a
  //     context named "main" with dimensions w×h. The caller OWNS the lifetime of
  //     `system` and must keep it alive until shutdown() or destruction.
  //     Returns true on success. On failure the object is left in a safe state for
  //     shutdown() or destruction.
  // PT: Configura interfaces de sistema+render do RmlUi, chama Rml::Initialise() e
  //     cria contexto "main" com dimensões w×h. O chamador é DONO do lifetime de
  //     `system` e deve mantê-lo vivo até shutdown() ou destruição.
  //     Retorna true em caso de sucesso. Em falha o objeto fica em estado seguro para
  //     shutdown() ou destruição.
  bool init(Rml::SystemInterface* system, RenderGl3& render, int w, int h);

  // EN: Load a document from rml_path and call Show(). Returns true on success.
  // PT: Carrega documento de rml_path e chama Show(). Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Override the base URL for asset resolution. When set, relative paths (fonts, images,
  //     RCSS) are resolved as base_url + '/' + path instead of relative to CWD.
  //     Pass nullptr or "" to clear.
  //     Safe to call after init(). Calling before init() is a no-op — the configuration
  //     is silently lost because impl_ does not yet exist.
  // PT: Sobrepõe o base URL para resolução de assets. Quando definido, caminhos relativos
  //     (fontes, imagens, RCSS) são resolvidos como base_url + '/' + path em vez de relativo ao CWD.
  //     Passe nullptr ou "" para limpar.
  //     Seguro chamar após init(). Chamar antes de init() é no-op — a configuração é
  //     silenciosamente perdida pois impl_ ainda não existe.
  void set_asset_base_url(const char* url);

  // EN: Returns the active Rml::Context (valid after successful init()), or nullptr.
  // PT: Retorna o Rml::Context ativo (válido após init() com sucesso), ou nullptr.
  Rml::Context* context();

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none). No ordering constraint versus
  //     load(): the listener reads the CURRENT callback at click time, not at attach time.
  //     Safe to call before or after load(); a no-op copy is stored even before init().
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum). Sem restrição de ordem vs.
  //     load(): o listener lê o callback CORRENTE no momento do clique, não no attach.
  //     Seguro chamar antes ou depois de load(); sem efeito antes de init() (impl_ ainda nulo).
  void set_click_callback(std::function<void(const char*)> cb);

  // EN: Query the border-box geometry of an element by id, in the LATEST loaded document's
  //     own content-local space (top-left origin, y-down, offset-free -- UiLayer/App translate
  //     to window-space at the public boundary). Returns false if no document is loaded or the
  //     id is not found; x/y/w/h are left untouched in that case.
  // PT: Consulta a geometria border-box de um elemento por id, no espaço LOCAL de conteúdo do
  //     último documento carregado (origem topo-esquerda, y pra baixo, offset-free -- UiLayer/
  //     App traduzem pra espaço-janela na fronteira pública). Retorna false se nenhum documento
  //     estiver carregado ou o id não for encontrado; x/y/w/h ficam intocados nesse caso.
  bool get_element_box(const char* id, float& x, float& y, float& w, float& h) const;

  // EN: Shutdown RmlUi and release all resources. Safe to call multiple times.
  //     Does NOT delete the SystemInterface — the caller owns it.
  // PT: Encerra o RmlUi e libera todos os recursos. Seguro chamar múltiplas vezes.
  //     NÃO deleta o SystemInterface — o chamador é o dono.
  void shutdown();

private:
  struct Impl;        // EN: defined in bootstrap.cpp; hides RmlUi types.
                      // PT: definido em bootstrap.cpp; oculta tipos RmlUi.
  Impl* impl_ = nullptr;
};

} // namespace glintfx
