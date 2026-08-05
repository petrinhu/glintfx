// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-0/F2 -- glintfx::Key/Mod_* -> Rml::Input::KeyIdentifier/KeyModifier translation,
//     extracted verbatim from engine.cpp (where it lived as two `static` free functions,
//     to_rml_key/to_rml_mods -- moved here without behaviour change) so engine.cpp no longer
//     needs to #include any RmlUi header at all. Internal header (src/rml/-only, NOT under
//     glintfx/include/glintfx/ -- the include-based encapsulation gate,
//     tools/check_encapsulation.sh, does not cover this file, same exemption type_bridge.hpp
//     already documents) -- consumed exclusively by bootstrap.cpp's process_key/
//     process_mouse_move/process_mouse_button/process_mouse_wheel (the new "driving" surface
//     RMLX-0/F2 added to Bootstrap so engine.cpp can route Rml::Context calls through it
//     instead of touching Rml::Context/Rml::Input directly).
//
//     This is the OTHER anticorruption seam RMLX-0 draws (type_bridge.hpp is the one for
//     value types, Vec2F/ColorF <-> Vector2f/Colourb) -- every glintfx::Key/Mod-vs-RmlUi
//     boundary should route through here, so a future RmlUi swap-out (RMLX-1..11) touches ONE
//     file's worth of key-mapping logic, not N call sites.
//
//     MUTATION-TESTED (RMLX-0/F2): tests/input_map_sanity.cpp asserts all 14 explicitly-mapped
//     keys against DISTINCT Rml::Input::KeyIdentifier values -- swapping any two case labels in
//     to_rml_key's switch below is guaranteed to turn at least one assertion red. See that
//     test file's own header comment for the full oracle and the swap this was proven against.
// PT: RMLX-0/F2 -- tradução de glintfx::Key/Mod_* para Rml::Input::KeyIdentifier/KeyModifier,
//     extraída tal-e-qual de engine.cpp (onde vivia como duas funções livres `static`,
//     to_rml_key/to_rml_mods -- movida para cá sem mudança de comportamento) para que
//     engine.cpp deixe de precisar incluir qualquer header do RmlUi. Header interno (só-
//     src/rml/, NÃO sob glintfx/include/glintfx/ -- o gate de encapsulamento include-based,
//     tools/check_encapsulation.sh, não cobre este arquivo, mesma isenção que type_bridge.hpp
//     já documenta) -- consumido exclusivamente por process_key/process_mouse_move/
//     process_mouse_button/process_mouse_wheel do bootstrap.cpp (a nova superfície de
//     "condução" que RMLX-0/F2 somou ao Bootstrap para que engine.cpp roteasse chamadas de
//     Rml::Context por ali em vez de tocar Rml::Context/Rml::Input diretamente).
//
//     Esta é a OUTRA costura anticorrupção que a RMLX-0 traça (type_bridge.hpp é a de tipos-
//     valor, Vec2F/ColorF <-> Vector2f/Colourb) -- toda fronteira glintfx::Key/Mod-vs-RmlUi
//     deveria passar por aqui, para que uma futura troca do RmlUi (RMLX-1..11) toque UM
//     arquivo de lógica de mapeamento de tecla, não N sítios de chamada.
//
//     TESTADO POR MUTAÇÃO (RMLX-0/F2): tests/input_map_sanity.cpp afirma as 14 teclas
//     explicitamente mapeadas contra valores Rml::Input::KeyIdentifier DISTINTOS -- trocar
//     quaisquer dois case labels no switch de to_rml_key abaixo garantidamente deixa vermelha
//     pelo menos uma asserção. Ver o próprio comentário de cabeçalho daquele arquivo de teste
//     pro oráculo completo e a troca contra a qual isto foi provado.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <glintfx/ui_event.hpp> // EN: glintfx::Key, Mod_Shift/Mod_Ctrl/Mod_Alt. PT: glintfx::Key, Mod_Shift/Mod_Ctrl/Mod_Alt.

#include <RmlUi/Core/Input.h> // EN: Rml::Input::KeyIdentifier/KeyModifier. PT: Rml::Input::KeyIdentifier/KeyModifier.

namespace glintfx {

// EN: Map glintfx::Key to Rml::Input::KeyIdentifier.
//     Gamepad nav: Up/Down/Left/Right -> arrow keys; Enter -> Return; Escape stays Escape.
//     Tab drives RmlUi's built-in focus cycle; Space/Backspace for text widget editing.
//     AUD-PUB-3 (v0.5.0): Delete/Home/End/PageUp/PageDown -> KI_DELETE/KI_HOME/KI_END/KI_PRIOR/
//     KI_NEXT -- names confirmed by grepping the pinned RmlUi source
//     (build/_deps/rmlui-src/Include/RmlUi/Core/Input.h:115-128; KI_PRIOR is Page Up,
//     KI_NEXT is Page Down -- RmlUi's own naming, not a typo here).
//     Unmapped Key values (Key::None, and every letter/digit/F-key/etc. appended after
//     Key::PageDown for HOSTIN-1's physical-input channel -- see ui_event.hpp) fall through to
//     the default branch, KI_UNKNOWN -- see that enum's own doc-comment for why letters/digits
//     are deliberately out of this nav-oriented translation's scope.
// PT: Mapeia glintfx::Key para Rml::Input::KeyIdentifier.
//     Nav por gamepad: Up/Down/Left/Right -> setas; Enter -> Return; Escape permanece Escape.
//     Tab aciona o ciclo de foco interno do RmlUi; Space/Backspace para edição em widget de texto.
//     AUD-PUB-3 (v0.5.0): Delete/Home/End/PageUp/PageDown -> KI_DELETE/KI_HOME/KI_END/KI_PRIOR/
//     KI_NEXT -- nomes confirmados grepando o source pinado do RmlUi
//     (build/_deps/rmlui-src/Include/RmlUi/Core/Input.h:115-128; KI_PRIOR é Page Up,
//     KI_NEXT é Page Down -- nomenclatura do próprio RmlUi, não é erro de digitação aqui).
//     Valores de Key não mapeados (Key::None, e toda letra/dígito/tecla-F/etc. anexada após
//     Key::PageDown pro canal de input físico do HOSTIN-1 -- ver ui_event.hpp) caem no branch
//     default, KI_UNKNOWN -- ver o próprio doc-comment daquele enum pro motivo de letras/dígitos
//     ficarem deliberadamente fora do escopo desta tradução orientada a navegação.
Rml::Input::KeyIdentifier to_rml_key(Key k) noexcept;

// EN: Map glintfx::Mod bitmask to Rml::Input::KeyModifier bitmask.
//     glintfx::Mod_Shift=1, Mod_Ctrl=2, Mod_Alt=4
//     Rml::Input::KM_CTRL=1, KM_SHIFT=2, KM_ALT=4
// PT: Mapeia bitmask de glintfx::Mod para bitmask de Rml::Input::KeyModifier.
int to_rml_mods(int mods) noexcept;

} // namespace glintfx
