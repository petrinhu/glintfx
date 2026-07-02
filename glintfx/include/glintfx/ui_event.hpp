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
    Resize        // EN: logical viewport resize (width, height in pixels).
                  // PT: redimensionamento lógico do viewport (width, height em pixels).
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
  float       x         = 0.f;          // EN: pointer X  (MouseMove). PT: X do ponteiro.
  float       y         = 0.f;          // EN: pointer Y  (MouseMove). PT: Y do ponteiro.
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
