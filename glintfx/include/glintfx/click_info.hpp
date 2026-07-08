// SPDX-License-Identifier: MPL-2.0
// EN: Plain result struct delivered to set_click_info_callback() -- a richer companion to the
//     id-only set_click_callback() (AUD-PUB-4, v0.5.0). Reports which element was clicked, which
//     mouse button, where, and whether it was part of a double-click -- everything the id-only
//     callback could not (right-click context menus, double-click-to-open, coordinate-aware
//     hit feedback for elements without an authored id).
//
//     SOURCE -- TWO PATHS, both feeding the SAME callback (AUD-PUB-4 gap closure, v0.5.0):
//       - LEFT (button 0): populated from the native Rml::EventId::Click / Rml::EventId::Dblclick
//         events (Rml::Event::GetParameter<int>("button", 0) / GetParameter<float>("mouse_x"/
//         "mouse_y", 0.f) -- names confirmed by grepping the pinned RmlUi source,
//         Source/Core/Context.cpp:1538-1541). Unchanged since the original AUD-PUB-4 landing.
//       - RIGHT/MIDDLE (button 1/2): RmlUi's own Context never dispatches Click/Dblclick for
//         non-primary buttons (confirmed reading Context::ProcessMouseButtonDown/Up in the
//         pinned source -- the `else` branch for button_index != 0 dispatches Mousedown/Mouseup
//         only). We SYNTHESIZE the click ourselves: a bootstrap.cpp-internal listener pair
//         remembers which element (ancestor-walk to nearest id, same walk as the Click path)
//         a non-primary Mousedown landed on; if the PAIRED Mouseup for the SAME button resolves
//         to the SAME element id, this callback fires once, using the Mouseup event's own
//         button/mouse_x/mouse_y. Down-on-A-drag-up-on-B does NOT fire (mirrors RmlUi's own
//         left-click rule: Context::ProcessMouseButtonUp only fires Click when
//         active==FindFocusElement(hover), i.e. the press target is still current at release).
//
//     KNOWN LIMITATION -- `double_click` is LEFT-ONLY: RmlUi has no Dblclick equivalent for
//     non-primary buttons (no native event to synthesize from, and inventing double-click
//     detection via a custom timer was explicitly rejected -- see set_click_info_callback's
//     doc-comment). A right/middle ClickInfo therefore always reports double_click=false, even
//     for two rapid right-clicks on the same element.
//
//     COORDINATE SPACE -- (x, y): SAME convention as get_element_box()/ElementBox: window/
//     render-target physical pixels, top-left origin, y-down. UiLayer::set_click_info_callback
//     translates the content-local coordinates RmlUi reports into this window-space convention
//     by adding the active sub-viewport offset (mirrors get_element_box's box.x = x + impl_->x
//     translation); App::set_click_info_callback needs no translation (App owns the whole
//     window, offset is always (0,0)). Applies identically to left and right/middle.
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
//     ORIGEM -- DOIS CAMINHOS, ambos alimentando o MESMO callback (fechamento de gap AUD-PUB-4,
//     v0.5.0):
//       - ESQUERDO (botão 0): populado a partir dos eventos nativos Rml::EventId::Click /
//         Rml::EventId::Dblclick (Rml::Event::GetParameter<int>("button", 0) /
//         GetParameter<float>("mouse_x"/"mouse_y", 0.f) -- nomes confirmados grepando o source
//         pinado do RmlUi, Source/Core/Context.cpp:1538-1541). Inalterado desde o AUD-PUB-4
//         original.
//       - DIREITO/MEIO (botão 1/2): o próprio Context do RmlUi nunca dispara Click/Dblclick
//         para botões não-primários (confirmado lendo Context::ProcessMouseButtonDown/Up no
//         source pinado -- o ramo `else` para button_index != 0 dispara só Mousedown/Mouseup).
//         SINTETIZAMOS o clique por conta própria: um par de listeners interno ao bootstrap.cpp
//         lembra em qual elemento (subida de ancestral até o id mais próximo, mesma subida do
//         caminho de Click) um Mousedown não-primário caiu; se o Mouseup PAREADO do MESMO botão
//         resolve para o MESMO id de elemento, este callback dispara uma vez, usando o próprio
//         botão/mouse_x/mouse_y do evento Mouseup. Down em A, arrasta, up em B NÃO dispara
//         (espelha a própria regra de clique esquerdo do RmlUi:
//         Context::ProcessMouseButtonUp só dispara Click quando
//         active==FindFocusElement(hover), ou seja, o alvo da pressão ainda é o corrente na
//         soltura).
//
//     LIMITAÇÃO CONHECIDA -- `double_click` é ESQUERDO-APENAS: o RmlUi não tem equivalente de
//     Dblclick para botões não-primários (nenhum evento nativo do qual sintetizar, e inventar
//     detecção de duplo-clique via timer customizado foi explicitamente rejeitada -- ver o
//     doc-comment de set_click_info_callback). Um ClickInfo de botão direito/meio portanto
//     sempre reporta double_click=false, mesmo para dois cliques direitos rápidos no mesmo
//     elemento.
//
//     ESPAÇO DE COORDENADAS -- (x, y): MESMA convenção de get_element_box()/ElementBox: pixels
//     físicos do render-target/janela, origem superior-esquerda, y pra baixo.
//     UiLayer::set_click_info_callback traduz as coordenadas content-local que o RmlUi reporta
//     para esta convenção espaço-janela somando o offset da sub-viewport ativa (espelha a
//     tradução box.x = x + impl_->x de get_element_box); App::set_click_info_callback não
//     precisa de tradução (App é dono da janela inteira, offset é sempre (0,0)). Aplica-se
//     identicamente a esquerdo e direito/meio.
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
  int         button = 0;      // EN: 0=left, 1=right, 2=middle -- functional for all three (v0.5.0):
                                //     left via native Click/Dblclick, right/middle synthesized from
                                //     Mousedown+Mouseup (see "SOURCE" above). Only double_click stays
                                //     left-only ("KNOWN LIMITATION" above).
                                // PT: 0=esquerdo, 1=direito, 2=meio -- funcional para os três (v0.5.0):
                                //     esquerdo via Click/Dblclick nativos, direito/meio sintetizado de
                                //     Mousedown+Mouseup (ver "ORIGEM" acima). Só double_click continua
                                //     esquerdo-apenas ("LIMITAÇÃO CONHECIDA" acima).
  float       x      = 0.f;    // EN: window-space physical-pixel X, same convention as ElementBox.
                                // PT: X em pixels físicos espaço-janela, mesma convenção do ElementBox.
  float       y      = 0.f;    // EN: window-space physical-pixel Y, same convention as ElementBox.
                                // PT: Y em pixels físicos espaço-janela, mesma convenção do ElementBox.
  bool        double_click = false;  // EN: true when this invocation came from Rml::EventId::Dblclick.
                                      // PT: true quando esta invocação veio de Rml::EventId::Dblclick.
};

} // namespace glintfx
