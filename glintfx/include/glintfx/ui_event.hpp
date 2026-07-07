// SPDX-License-Identifier: MPL-2.0
// EN: Neutral, host-agnostic input event. The host fills this struct and calls
//     UiLayer::process_event(); glintfx translates it to the UI engine internally.
//     No third-party types appear here — the public API is fully decoupled from any
//     windowing, input, or rendering backend.
//     Gamepad nav: map d-pad/buttons → Key::Up/Down/Left/Right/Enter/Escape so that the
//     focus navigation of the UI engine (arrows / Return) is driven from any input source.
// PT: Evento de entrada neutro, agnóstico de host. O host preenche esta struct e chama
//     UiLayer::process_event(); o glintfx traduz para o motor de UI internamente.
//     Nenhum tipo de terceiros aparece aqui — a API pública é completamente desacoplada
//     de qualquer backend de janela, input ou renderização.
//     Nav por gamepad: mapeie d-pad/botões → Key::Up/Down/Left/Right/Enter/Escape para
//     conduzir a navegação de foco do motor de UI (setas/Return) de qualquer fonte de input.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

// EN: Logical key identifiers — navigation-oriented subset. Extend for text editing as needed.
// PT: Identificadores lógicos de tecla — subconjunto orientado a navegação. Extender conforme necessário.
enum class Key {
  None,
  Up,        // EN: d-pad up / arrow up.     PT: d-pad cima / seta acima.
  Down,      // EN: d-pad down / arrow down.  PT: d-pad baixo / seta abaixo.
  Left,      // EN: d-pad left / arrow left.  PT: d-pad esquerda / seta esquerda.
  Right,     // EN: d-pad right / arrow right. PT: d-pad direita / seta direita.
  Enter,     // EN: d-pad confirm / Return.   PT: d-pad confirmar / Return.
  Escape,    // EN: cancel / back button.     PT: cancelar / botão voltar.
  Tab,       // EN: focus cycle.              PT: ciclo de foco.
  Space,     // EN: space bar / face button.  PT: barra de espaço / botão face.
  Backspace  // EN: delete backward.          PT: apagar para trás.
};

// EN: Keyboard/gamepad modifier bitmask — combine with bitwise-OR.
// PT: Bitmask de modificadores de teclado/gamepad — combinar com bitwise-OR.
enum Mod {
  Mod_None  = 0,
  Mod_Shift = 1,
  Mod_Ctrl  = 2,
  Mod_Alt   = 4
};

// EN: Host-agnostic input event. Use designated initializers (C++20) for clarity:
//       UiEvent{ .type = UiEvent::Type::MouseMove, .x = 100.f, .y = 80.f }
// PT: Evento de entrada agnóstico de host. Use inicializadores designados (C++20) para clareza.
struct UiEvent {
  enum class Type {
    // EN: No event (L1.10-APIDOC). The default-constructed value: `UiEvent{}` (no designated
    //     initializer for `.type`) is inert and process_event() drops it as a no-op. Chosen as
    //     the default instead of MouseMove so a caller that forgets to set `.type` gets an
    //     obviously-inert event rather than a silent phantom pointer move to (0, 0).
    // PT: Nenhum evento (L1.10-APIDOC). Valor default-construído: `UiEvent{}` (sem
    //     inicializador designado para `.type`) é inerte e process_event() descarta como
    //     no-op. Escolhido como default em vez de MouseMove para que um chamador que esqueça
    //     de definir `.type` receba um evento obviamente inerte, não um movimento fantasma
    //     silencioso do ponteiro para (0, 0).
    None,
    MouseMove,    // EN: pointer position update (x, y in pixels).
                  // PT: atualização de posição do ponteiro (x, y em pixels).
    MouseButton,  // EN: button press/release (button, pressed).
                  // PT: pressão/liberação de botão (button, pressed).
    Key,          // EN: keyboard/gamepad key (key, modifiers, pressed).
                  // PT: tecla de teclado/gamepad (key, modifiers, pressed).
    Text,         // EN: unicode text input (text — null-terminated UTF-8, lifetime ≥ call).
                  // PT: entrada de texto unicode (text — UTF-8 terminado em '\0', lifetime ≥ chamada).
    Resize,       // EN: logical viewport resize (width, height in pixels).
                  // PT: redimensionamento lógico do viewport (width, height em pixels).
    // EN: Mouse-wheel / trackpad scroll (x, y — delta, NOT position; see the field comments
    //     below). GLINTFX-SCROLL-1, v0.3.2.
    //
    //     WHICH ELEMENT SCROLLS — HOVER, NOT FOCUS (confirmed by reading the pinned RmlUi
    //     source, examples/RmlUi/Source/Core/Context.cpp): Context::ProcessMouseWheel(Vector2f,
    //     int) (line 804) reads the `hover` member — the element under the pointer, tracked
    //     internally by the most recent ProcessMouseMove/ProcessMouseButtonDown call, NOT the
    //     focused/keyboard-active element — and is a safe, crash-free no-op when hover is null
    //     (line 811: `else if (!hover) { scroll_controller->Reset(); return true; }`). When
    //     hover IS set, the actual scroll target is resolved from it via
    //     `Element* target = hover->GetClosestScrollableContainer()` (line 829), which walks
    //     UP the ancestor chain from the hovered element looking for the nearest overflow-
    //     auto/scroll container; if NONE of hover's ancestors are scrollable,
    //     GetClosestScrollableContainer() itself returns nullptr (confirmed in
    //     examples/RmlUi/Source/Core/Element.cpp) and the wheel event is ALSO a safe no-op —
    //     it never scrolls an unrelated ancestor "by accident". PRACTICAL CONSEQUENCE: a
    //     MouseWheel event with no preceding MouseMove over the target area is silently
    //     dropped (hover is whatever the LAST MouseMove left it as, possibly none) — send a
    //     MouseMove to the desired position immediately BEFORE MouseWheel if the host does not
    //     already track hover via its own per-frame move events:
    //       ui.process_event({ .type = UiEvent::Type::MouseMove, .x = mouse_x, .y = mouse_y });
    //       ui.process_event({ .type = UiEvent::Type::MouseWheel, .x = 0.f, .y = wheel_dy });
    //     Focus (keyboard/gamepad selection, Rml::Context's `focus`/FindFocusElement) is a
    //     SEPARATE concept entirely unused by ProcessMouseWheel — a focused-but-not-hovered
    //     list (e.g. selected via gamepad, cursor elsewhere) does NOT receive wheel scroll;
    //     use scroll_element_into_view()/set_element_scroll_top() to follow a focus-driven
    //     selection programmatically instead (see UiLayer::scroll_element_into_view).
    // PT: Rolagem de roda-do-mouse / trackpad (x, y — delta, NÃO posição; ver os comentários
    //     dos campos abaixo). GLINTFX-SCROLL-1, v0.3.2.
    //
    //     QUAL ELEMENTO ROLA — HOVER, NÃO FOCO (confirmado lendo o source pinado do RmlUi,
    //     examples/RmlUi/Source/Core/Context.cpp): Context::ProcessMouseWheel(Vector2f, int)
    //     (linha 804) lê o membro `hover` — o elemento sob o ponteiro, rastreado internamente
    //     pela chamada ProcessMouseMove/ProcessMouseButtonDown mais recente, NÃO o elemento com
    //     foco/ativo por teclado — e é um no-op seguro e sem crash quando hover é nulo (linha
    //     811: `else if (!hover) { scroll_controller->Reset(); return true; }`). Quando hover
    //     ESTÁ definido, o alvo real de rolagem é resolvido a partir dele via
    //     `Element* target = hover->GetClosestScrollableContainer()` (linha 829), que sobe a
    //     cadeia de ancestrais a partir do elemento em hover procurando o container overflow-
    //     auto/scroll mais próximo; se NENHUM ancestral de hover for rolável,
    //     GetClosestScrollableContainer() em si retorna nullptr (confirmado em
    //     examples/RmlUi/Source/Core/Element.cpp) e o evento de wheel TAMBÉM é um no-op seguro
    //     -- nunca rola um ancestral não-relacionado "por acidente". CONSEQUÊNCIA PRÁTICA: um
    //     evento MouseWheel sem um MouseMove precedente sobre a área-alvo é descartado
    //     silenciosamente (hover é o que o ÚLTIMO MouseMove deixou, possivelmente nenhum) —
    //     envie um MouseMove para a posição desejada IMEDIATAMENTE ANTES do MouseWheel se o
    //     host não já rastrear hover via seus próprios eventos de move por-frame:
    //       ui.process_event({ .type = UiEvent::Type::MouseMove, .x = mouse_x, .y = mouse_y });
    //       ui.process_event({ .type = UiEvent::Type::MouseWheel, .x = 0.f, .y = wheel_dy });
    //     Foco (seleção por teclado/gamepad, `focus`/FindFocusElement do Rml::Context) é um
    //     conceito TOTALMENTE SEPARADO, nunca usado por ProcessMouseWheel — uma lista com foco
    //     mas sem hover (ex.: selecionada via gamepad, cursor em outro lugar) NÃO recebe
    //     rolagem de wheel; use scroll_element_into_view()/set_element_scroll_top() para seguir
    //     uma seleção guiada por foco programaticamente em vez disso (ver
    //     UiLayer::scroll_element_into_view).
    MouseWheel
  };

  // EN: L1.10-APIDOC: default changed from MouseMove to None (breaking change to an already
  //     shipped API — v0.2.4 and earlier default to MouseMove). Rationale: `UiEvent{}` looked
  //     harmless but was silently a mouse-move-to-origin event; None makes an unset .type
  //     inert instead. Hosts that build events via designated initializers with an explicit
  //     `.type = ...` (the documented usage pattern; see the struct comment above and every
  //     call site in tests/ui_layer_events.cpp) are unaffected.
  // PT: L1.10-APIDOC: default trocado de MouseMove para None (mudança de comportamento numa
  //     API já shipada — v0.2.4 e anteriores usavam MouseMove como default). Motivo:
  //     `UiEvent{}` parecia inofensivo mas era silenciosamente um evento de mouse-move para a
  //     origem; None torna um .type não definido inerte. Hosts que constroem eventos via
  //     inicializadores designados com `.type = ...` explícito (padrão de uso documentado; ver
  //     o comentário da struct acima e todos os call sites em tests/ui_layer_events.cpp) não
  //     são afetados.
  Type        type      = Type::None;
  // EN: Dual-purpose, like width/height below for Resize: pointer POSITION for MouseMove
  //     (pixels, window-space), but wheel/scroll DELTA for MouseWheel (arbitrary units — one
  //     "notch" of a traditional wheel is conventionally ~1.0; trackpads may send fractional
  //     values). MouseWheel convention mirrors Rml::Context::ProcessMouseWheel: positive x
  //     scrolls right, positive y scrolls down (see Context.h in the pinned RmlUi source).
  // PT: Duplo propósito, como width/height abaixo para Resize: POSIÇÃO do ponteiro no
  //     MouseMove (pixels, espaço-janela), mas DELTA de rolagem no MouseWheel (unidades
  //     arbitrárias — um "clique" de roda tradicional é convencionalmente ~1.0; trackpads
  //     podem enviar valores fracionários). Convenção do MouseWheel espelha
  //     Rml::Context::ProcessMouseWheel: x positivo rola para a direita, y positivo rola
  //     para baixo (ver Context.h no source pinado do RmlUi).
  float       x         = 0.f;          // EN: pointer X (MouseMove) / wheel delta X (MouseWheel).
                                        // PT: X do ponteiro (MouseMove) / delta X de roda (MouseWheel).
  float       y         = 0.f;          // EN: pointer Y (MouseMove) / wheel delta Y (MouseWheel).
                                        // PT: Y do ponteiro (MouseMove) / delta Y de roda (MouseWheel).
  int         button    = 0;            // EN: 0=left,1=right,2=middle (MouseButton).
                                        // PT: 0=esquerdo,1=direito,2=meio.
  bool        pressed   = false;        // EN: button/key down? (MouseButton, Key).
                                        // PT: botão/tecla pressionado?
  Key         key       = Key::None;    // EN: logical key (Key events).
                                        // PT: tecla lógica (eventos Key).
  int         modifiers = Mod_None;     // EN: bitmask of Mod flags. PT: bitmask de flags Mod.
  const char* text      = nullptr;      // EN: UTF-8 text (Text events; caller owns lifetime).
                                        // PT: texto UTF-8 (eventos Text; lifetime do chamador).
  int         width     = 0;            // EN: new viewport width  (Resize). PT: nova largura.
  int         height    = 0;            // EN: new viewport height (Resize). PT: nova altura.
};

} // namespace glintfx
