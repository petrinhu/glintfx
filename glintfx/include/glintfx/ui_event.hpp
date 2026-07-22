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

// EN: Logical key identifiers — navigation-oriented subset, extended (AUD-PUB-3, v0.5.0) with
//     the 5 text-editing/document-navigation keys RmlUi already supports natively
//     (KI_DELETE/KI_HOME/KI_END/KI_PRIOR/KI_NEXT, confirmed by grepping the pinned RmlUi source
//     — see to_rml_key() in ui_layer.cpp). Letters/digits (Ctrl+C/V/Z shortcuts) are a larger,
//     separate scope (would also need a UiEvent::Type::Text-style codepoint path for real text
//     input) and were deliberately deferred — see the AUD-PUB-6 INBOX seed in TODO.md.
// PT: Identificadores lógicos de tecla — subconjunto orientado a navegação, estendido
//     (AUD-PUB-3, v0.5.0) com as 5 teclas de edição de texto/navegação de documento que o
//     RmlUi já suporta nativamente (KI_DELETE/KI_HOME/KI_END/KI_PRIOR/KI_NEXT, confirmado
//     grepando o source pinado do RmlUi — ver to_rml_key() em ui_layer.cpp). Letras/dígitos
//     (atalhos Ctrl+C/V/Z) são um escopo maior e separado (exigiria também um caminho de
//     codepoint estilo UiEvent::Type::Text para entrada de texto real) e foram deliberadamente
//     adiados — ver a semente INBOX AUD-PUB-6 no TODO.md.
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
  Backspace, // EN: delete backward.          PT: apagar para trás.
  Delete,    // EN: delete forward.           PT: apagar para frente (Del).
  Home,      // EN: line/document start.      PT: início de linha/documento.
  End,       // EN: line/document end.        PT: fim de linha/documento.
  PageUp,    // EN: scroll/page up.           PT: rolar/página acima.
  PageDown,  // EN: scroll/page down.         PT: rolar/página abaixo.

  // EN: HOSTIN-1 (Onda 2, v0.19.0) -- APPEND-ONLY extension for the host's PHYSICAL input
  //     state channel (App::is_key_down/App::set_key_callback -- docs/superpowers/plans/
  //     2026-07-22-onda2-input-host.md, decision D1). Mirrors GLFW's own nameable key set so a
  //     game can read WASD/letters/digits/F-keys/modifiers/numpad/punctuation directly, without
  //     a second key vocabulary for the whole library. The pre-existing entries above
  //     (None..PageDown) keep their EXACT numeric values forever -- enforced by the
  //     static_asserts immediately below this enum -- and every entry from here on is appended
  //     AFTER them, ending with the Count sentinel: Count is NEVER a real key (never emitted in
  //     a UiEvent, an edge callback, or an is_key_down query); any FUTURE key addition goes
  //     BEFORE Count, never after it. These tokens are POSITION-based (named after the physical
  //     key on a US layout), not label-based -- see glfw_translate_key's doc-comment
  //     (src/glfw_event_translate.hpp) for the ABNT2 consequences (ç arrives as the Semicolon
  //     position; the second "/" key -- scancode 97 -- has no GLFW token at all, a documented
  //     upstream gap; World1/World2 cover the ISO extra key some ABNT2/European boards report).
  // PT: HOSTIN-1 (Onda 2, v0.19.0) -- extensão APPEND-ONLY para o canal de estado de input
  //     FÍSICO do host (App::is_key_down/App::set_key_callback -- docs/superpowers/plans/
  //     2026-07-22-onda2-input-host.md, decisão D1). Espelha o próprio conjunto nomeável de
  //     teclas do GLFW para que um jogo leia WASD/letras/dígitos/F-teclas/modificadores/numpad/
  //     pontuação diretamente, sem um segundo vocabulário de tecla pra biblioteca inteira. As
  //     entradas pré-existentes acima (None..PageDown) mantêm seus valores numéricos EXATOS pra
  //     sempre -- reforçado pelos static_asserts logo abaixo deste enum -- e toda entrada a
  //     partir daqui é anexada DEPOIS delas, terminando no sentinela Count: Count NUNCA é uma
  //     tecla de verdade (nunca emitido numa UiEvent, num callback de edge, ou numa consulta
  //     is_key_down); qualquer adição FUTURA de tecla entra ANTES de Count, nunca depois. Estes
  //     tokens são baseados em POSIÇÃO (nomeados pela tecla física de um layout US), não em
  //     rótulo -- ver o doc-comment de glfw_translate_key (src/glfw_event_translate.hpp) para as
  //     consequências no ABNT2 (ç chega como a posição Semicolon; a segunda tecla "/" --
  //     scancode 97 -- não tem token GLFW nenhum, uma lacuna upstream documentada; World1/World2
  //     cobrem a tecla extra ISO que alguns teclados ABNT2/europeus reportam).
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Digit0,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  Digit6,
  Digit7,
  Digit8,
  Digit9,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  LeftShift,
  RightShift,
  LeftControl,
  RightControl,
  LeftAlt,
  RightAlt,
  LeftSuper,
  RightSuper,
  Insert,
  CapsLock,
  ScrollLock,
  NumLock,
  PrintScreen,
  Pause,
  Menu,
  Apostrophe,
  Comma,
  Minus,
  Period,
  Slash,
  Semicolon,
  Equal,
  LeftBracket,
  Backslash,
  RightBracket,
  GraveAccent,
  World1,
  World2,
  Kp0,
  Kp1,
  Kp2,
  Kp3,
  Kp4,
  Kp5,
  Kp6,
  Kp7,
  Kp8,
  Kp9,
  KpDecimal,
  KpDivide,
  KpMultiply,
  KpSubtract,
  KpAdd,
  KpEqual,

  // EN: Sentinel -- one past the last real key. Never a valid key value on its own; used only
  //     for array sizing (src/input_state.hpp) and range checks. NEVER add an enumerator after
  //     this one -- new keys are inserted BEFORE it (see this enum's own doc-comment above).
  // PT: Sentinela -- um após a última tecla de verdade. Nunca um valor de tecla válido por si
  //     só; usado só para dimensionar array (src/input_state.hpp) e checagens de faixa. NUNCA
  //     adicione um enumerador depois deste -- teclas novas entram ANTES dele (ver o próprio
  //     doc-comment deste enum acima).
  Count
};

// EN: D1 append-only freeze (HOSTIN-1) -- these MUST equal today's numeric values forever. If
//     any of these fails to compile, someone inserted/removed/reordered an enumerator inside
//     None..PageDown, or shuffled the append order -- both forbidden by this enum's own
//     doc-comment above. Machine-enforced so the append-only rule is never just "reviewed by
//     eye" (mutation-testability note for the adversarial reviewer: reordering PageUp/PageDown,
//     or inserting a new key before PageDown instead of after it, MUST turn one of these red).
// PT: Congelamento append-only do D1 (HOSTIN-1) -- estes PRECISAM valer os valores numéricos de
//     hoje pra sempre. Se algum destes falhar ao compilar, alguém inseriu/removeu/reordenou um
//     enumerador dentro de None..PageDown, ou embaralhou a ordem de anexação -- ambos proibidos
//     pelo próprio doc-comment deste enum acima. Reforçado por máquina para que a regra
//     append-only nunca seja só "revisada a olho" (nota de mutation-testability pro reviewer
//     adversarial: reordenar PageUp/PageDown, ou inserir uma tecla nova antes de PageDown em vez
//     de depois, TEM que deixar algum destes vermelho).
static_assert(static_cast<int>(Key::None) == 0, "Key::None must stay 0 (append-only, D1)");
static_assert(static_cast<int>(Key::Up) == 1, "Key::Up must stay 1 (append-only, D1)");
static_assert(static_cast<int>(Key::Down) == 2, "Key::Down must stay 2 (append-only, D1)");
static_assert(static_cast<int>(Key::Left) == 3, "Key::Left must stay 3 (append-only, D1)");
static_assert(static_cast<int>(Key::Right) == 4, "Key::Right must stay 4 (append-only, D1)");
static_assert(static_cast<int>(Key::Enter) == 5, "Key::Enter must stay 5 (append-only, D1)");
static_assert(static_cast<int>(Key::Escape) == 6, "Key::Escape must stay 6 (append-only, D1)");
static_assert(static_cast<int>(Key::Tab) == 7, "Key::Tab must stay 7 (append-only, D1)");
static_assert(static_cast<int>(Key::Space) == 8, "Key::Space must stay 8 (append-only, D1)");
static_assert(static_cast<int>(Key::Backspace) == 9, "Key::Backspace must stay 9 (append-only, D1)");
static_assert(static_cast<int>(Key::Delete) == 10, "Key::Delete must stay 10 (append-only, D1)");
static_assert(static_cast<int>(Key::Home) == 11, "Key::Home must stay 11 (append-only, D1)");
static_assert(static_cast<int>(Key::End) == 12, "Key::End must stay 12 (append-only, D1)");
static_assert(static_cast<int>(Key::PageUp) == 13, "Key::PageUp must stay 13 (append-only, D1)");
static_assert(static_cast<int>(Key::PageDown) == 14, "Key::PageDown must stay 14 (append-only, D1)");

// EN: Keyboard/gamepad modifier bitmask — combine with bitwise-OR.
// PT: Bitmask de modificadores de teclado/gamepad — combinar com bitwise-OR.
enum Mod {
  Mod_None  = 0,
  Mod_Shift = 1,
  Mod_Ctrl  = 2,
  Mod_Alt   = 4
};

// EN: Edge-detected key action (HOSTIN-2, Onda 2) -- App::set_key_callback's payload. Press and
//     Release each fire exactly once per physical transition; Repeat fires once per OS
//     auto-repeat tick while a key stays held (GLFW_REPEAT), forwarded verbatim -- most games
//     only care about Press/Release, Repeat is here for completeness/parity with GLFW's own
//     action space, never synthesised for anything glintfx itself does not receive from GLFW.
// PT: Ação de tecla edge-detectada (HOSTIN-2, Onda 2) -- payload de App::set_key_callback.
//     Press e Release disparam cada um exatamente uma vez por transição física; Repeat dispara
//     uma vez por tick de auto-repeat do SO enquanto uma tecla continua segurada (GLFW_REPEAT),
//     repassado tal-e-qual -- a maioria dos jogos só se importa com Press/Release, Repeat está
//     aqui por completude/paridade com o próprio espaço de ação do GLFW, nunca sintetizado para
//     nada que a própria glintfx não receba do GLFW.
enum class KeyAction { Press,
                       Release,
                       Repeat };

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
    //     below). GLINTFX-SCROLL-1, v0.4.0.
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
    //     dos campos abaixo). GLINTFX-SCROLL-1, v0.4.0.
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
