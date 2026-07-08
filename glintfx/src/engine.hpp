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
#include <glintfx/click_info.hpp>

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

  // EN: Compose-only frame path. (offset_x, offset_y) are OpenGL-native (bottom-left-origin)
  //     viewport offset -- ALREADY CONVERTED by the caller (UiLayer). Engine stays offset-space
  //     agnostic: it never interprets these values, only forwards them.
  // PT: Caminho compose-only. (offset_x, offset_y) são offset de viewport NATIVO do OpenGL
  //     (origem inferior-esquerda) -- JÁ CONVERTIDO pelo chamador (UiLayer). O Engine
  //     permanece agnóstico ao espaço do offset: nunca interpreta esses valores, só repassa.
  void render_compose(int offset_x, int offset_y, int w, int h);

  // EN: Returns the active Rml::Context, or nullptr if not ok().
  // PT: Retorna o Rml::Context ativo, ou nullptr se não ok().
  Rml::Context* context();

  // EN: Register a click callback -- forwards to Bootstrap::set_click_callback (F1, v0.2.5).
  //     See Bootstrap::set_click_callback for the full ordering/lifetime contract.
  // PT: Registra um callback de clique -- encaminha a Bootstrap::set_click_callback (F1,
  //     v0.2.5). Ver Bootstrap::set_click_callback para o contrato completo de ordem/lifetime.
  void set_click_callback(std::function<void(const char*)> cb);

  // EN: Register a richer click callback -- forwards to Bootstrap::set_click_info_callback
  //     (AUD-PUB-4, v0.5.0). Straight passthrough, no coordinate translation here -- x/y stay
  //     in Bootstrap's content-local space; UiLayer/App translate to window-space at the public
  //     boundary (see Bootstrap::set_click_info_callback / ClickInfo doc-comment).
  // PT: Registra um callback de clique mais rico -- encaminha a
  //     Bootstrap::set_click_info_callback (AUD-PUB-4, v0.5.0). Repasse direto, sem tradução de
  //     coordenada aqui -- x/y permanecem no espaço local de conteúdo do Bootstrap; UiLayer/App
  //     traduzem para espaço-janela na fronteira pública (ver Bootstrap::set_click_info_callback
  //     / doc-comment de ClickInfo).
  void set_click_info_callback(std::function<void(const ClickInfo&)> cb);

  // EN: Query the border-box geometry of an element by id -- forwards to
  //     Bootstrap::get_element_box (F2, v0.2.5). Content-local space (offset-free); UiLayer/
  //     App translate to window-space at the public boundary. Returns false when not ok(),
  //     no document is loaded, or the id is not found; x/y/w/h are left untouched in that case.
  // PT: Consulta a geometria border-box de um elemento por id -- encaminha a
  //     Bootstrap::get_element_box (F2, v0.2.5). Espaço local de conteúdo (offset-free);
  //     UiLayer/App traduzem para espaço-janela na fronteira pública. Retorna false quando
  //     não ok(), nenhum documento estiver carregado, ou o id não for encontrado; x/y/w/h
  //     ficam intocados nesse caso.
  bool get_element_box(const char* id, float& x, float& y, float& w, float& h) const;

  // EN: Scroll an element's scrolling ancestor(s) into view -- forwards to
  //     Bootstrap::scroll_element_into_view (GLINTFX-SCROLL-1, v0.4.0). Instant, synchronous.
  //     Returns false when not ok(), no document is loaded, or the id is not found.
  // PT: Rola o(s) ancestral(is) rolável(eis) de um elemento até ele ficar visível -- encaminha a
  //     Bootstrap::scroll_element_into_view (GLINTFX-SCROLL-1, v0.4.0). Instantâneo, síncrono.
  //     Retorna false quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query an element's own vertical scroll offset -- forwards to
  //     Bootstrap::get_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). Returns false (out param
  //     untouched) when not ok(), no document is loaded, or the id is not found.
  // PT: Consulta o offset de rolagem vertical próprio de um elemento -- encaminha a
  //     Bootstrap::get_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). Retorna false (out param
  //     intocado) quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query an element's total scrollable content height -- forwards to
  //     Bootstrap::get_element_scroll_height (GLINTFX-SCROLL-1, v0.4.0). Returns false (out
  //     param untouched) when not ok(), no document is loaded, or the id is not found.
  // PT: Consulta a altura total do conteúdo rolável de um elemento -- encaminha a
  //     Bootstrap::get_element_scroll_height (GLINTFX-SCROLL-1, v0.4.0). Retorna false (out
  //     param intocado) quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query an element's client (visible) height -- forwards to
  //     Bootstrap::get_element_client_height (GLINTFX-SCROLL-1, v0.4.0). Returns false (out
  //     param untouched) when not ok(), no document is loaded, or the id is not found.
  // PT: Consulta a altura de cliente (visível) de um elemento -- encaminha a
  //     Bootstrap::get_element_client_height (GLINTFX-SCROLL-1, v0.4.0). Retorna false (out
  //     param intocado) quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set an element's own vertical scroll offset -- forwards to
  //     Bootstrap::set_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). RmlUi clamps internally;
  //     this layer rejects non-finite input. Returns false when not ok(), no document is
  //     loaded, the id is not found, or scroll_top is not finite.
  // PT: Define o offset de rolagem vertical próprio de um elemento -- encaminha a
  //     Bootstrap::set_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). O RmlUi satura
  //     internamente; esta camada rejeita entrada não-finita. Retorna false quando não ok(),
  //     nenhum documento estiver carregado, o id não for encontrado, ou scroll_top não for
  //     finito.
  bool set_element_scroll_top(const char* id, float scroll_top) const;

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
