// SPDX-License-Identifier: MPL-2.0
// EN: Plain result struct delivered to set_click_info_callback() -- a richer companion to the
//     id-only set_click_callback() (AUD-PUB-4, v0.5.0). Reports which element was clicked, which
//     mouse button, where, and whether it was part of a double-click -- everything the id-only
//     callback could not (right-click context menus, double-click-to-open, coordinate-aware
//     hit feedback for elements without an authored id).
//
//     SOURCE: populated from the SAME native Rml::EventId::Click / Rml::EventId::Dblclick
//     events the id-only callback listens to (Rml::Event::GetParameter<int>("button", 0) /
//     GetParameter<float>("mouse_x"/"mouse_y", 0.f) -- names confirmed by grepping the pinned
//     RmlUi source, Source/Core/Context.cpp:1538-1541). NOT invented via a custom timer or a
//     Mousedown/Mouseup listener of our own.
//
//     KNOWN LIMITATION -- `button` is effectively always 0 (left): RmlUi's own Context only
//     ever DISPATCHES EventId::Click/Dblclick for the PRIMARY (left, button_index == 0) mouse
//     button (confirmed by reading Context::ProcessMouseButtonDown/Up in the pinned source --
//     the `else` branches for button_index != 0 dispatch Mousedown/Mouseup only, never Click).
//     So while this field exists and is read honestly off the native event, a right mouse
//     button press will simply never produce a ClickInfo callback invocation at all through
//     this path -- there is no RmlUi-native "right-click" event to surface. A host that needs
//     real right-click / context-menu detection must build it itself on top of a lower-level
//     event (Mousedown/Mouseup with button != 0) -- out of scope here; register as an INBOX
//     seed if/when a consumer needs it, same pull-incremental policy as AUD-PUB-6.
//
//     COORDINATE SPACE -- (x, y): SAME convention as get_element_box()/ElementBox: window/
//     render-target physical pixels, top-left origin, y-down. UiLayer::set_click_info_callback
//     translates the content-local coordinates RmlUi reports into this window-space convention
//     by adding the active sub-viewport offset (mirrors get_element_box's box.x = x + impl_->x
//     translation); App::set_click_info_callback needs no translation (App owns the whole
//     window, offset is always (0,0)).
//
//     LIFETIME -- `id` points into the clicked Rml::Element's own id string, valid only for the
//     duration of the callback invocation (same contract as set_click_callback's `element_id`
//     parameter). Copy it (e.g. into a std::string) if you need it afterwards.
//
// PT: Struct de resultado simples entregue a set_click_info_callback() -- um complemento mais
//     rico ao set_click_callback() só-id (AUD-PUB-4, v0.5.0). Reporta qual elemento foi
//     clicado, qual botão do mouse, onde, e se fez parte de um duplo-clique -- tudo que o
//     callback só-id não conseguia (menu de contexto por botão direito, abrir-com-duplo-clique,
//     feedback de hit com coordenada para elementos sem id autorado).
//
//     ORIGEM: populada a partir dos MESMOS eventos nativos Rml::EventId::Click /
//     Rml::EventId::Dblclick que o callback só-id escuta (Rml::Event::GetParameter<int>
//     ("button", 0) / GetParameter<float>("mouse_x"/"mouse_y", 0.f) -- nomes confirmados
//     grepando o source pinado do RmlUi, Source/Core/Context.cpp:1538-1541). NÃO inventada via
//     timer customizado nem via listener de Mousedown/Mouseup próprio.
//
//     LIMITAÇÃO CONHECIDA -- `button` é, na prática, sempre 0 (esquerdo): o próprio Context do
//     RmlUi só DISPARA EventId::Click/Dblclick para o botão PRIMÁRIO (esquerdo, button_index ==
//     0) do mouse (confirmado lendo Context::ProcessMouseButtonDown/Up no source pinado -- os
//     ramos `else` para button_index != 0 disparam só Mousedown/Mouseup, nunca Click). Então,
//     embora este campo exista e seja lido honestamente do evento nativo, uma pressão do botão
//     direito simplesmente NUNCA produz uma invocação de callback ClickInfo por este caminho --
//     não existe evento "right-click" nativo do RmlUi para expor. Um host que precise de
//     detecção real de botão direito/menu de contexto precisa construí-la por conta própria
//     sobre um evento de nível mais baixo (Mousedown/Mouseup com button != 0) -- fora de escopo
//     aqui; registrar como semente INBOX se/quando um consumidor precisar, mesma política pull
//     incremental do AUD-PUB-6.
//
//     ESPAÇO DE COORDENADAS -- (x, y): MESMA convenção de get_element_box()/ElementBox: pixels
//     físicos do render-target/janela, origem superior-esquerda, y pra baixo.
//     UiLayer::set_click_info_callback traduz as coordenadas content-local que o RmlUi reporta
//     para esta convenção espaço-janela somando o offset da sub-viewport ativa (espelha a
//     tradução box.x = x + impl_->x de get_element_box); App::set_click_info_callback não
//     precisa de tradução (App é dono da janela inteira, offset é sempre (0,0)).
//
//     LIFETIME -- `id` aponta para dentro da string de id do próprio Rml::Element clicado,
//     válido só durante a invocação do callback (mesmo contrato do parâmetro `element_id` de
//     set_click_callback). Copie (ex.: para um std::string) se precisar dele depois.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

struct ClickInfo {
  const char* id     = "";     // EN: nearest ancestor-or-self id, "" if none (see lifetime note above).
                                // PT: id do ancestral-ou-o-próprio mais próximo, "" se nenhum (ver nota de lifetime acima).
  int         button = 0;      // EN: 0=left, 1=right, 2=middle -- see "KNOWN LIMITATION" above:
                                //     effectively always 0 through this native-event path.
                                // PT: 0=esquerdo, 1=direito, 2=meio -- ver "LIMITAÇÃO CONHECIDA"
                                //     acima: na prática sempre 0 por este caminho de evento nativo.
  float       x      = 0.f;    // EN: window-space physical-pixel X, same convention as ElementBox.
                                // PT: X em pixels físicos espaço-janela, mesma convenção do ElementBox.
  float       y      = 0.f;    // EN: window-space physical-pixel Y, same convention as ElementBox.
                                // PT: Y em pixels físicos espaço-janela, mesma convenção do ElementBox.
  bool        double_click = false;  // EN: true when this invocation came from Rml::EventId::Dblclick.
                                      // PT: true quando esta invocação veio de Rml::EventId::Dblclick.
};

} // namespace glintfx
