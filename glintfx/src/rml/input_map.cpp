// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-0/F2 -- implementation of the glintfx::Key/Mod_* -> Rml::Input translation. Moved
//     verbatim from engine.cpp -- see input_map.hpp's own doc-comment for the full rationale
//     and the mutation-testing coverage in tests/input_map_sanity.cpp.
// PT: RMLX-0/F2 -- implementação da tradução glintfx::Key/Mod_* -> Rml::Input. Movida tal-e-qual
//     de engine.cpp -- ver o próprio doc-comment de input_map.hpp pro racional completo e a
//     cobertura de mutation-testing em tests/input_map_sanity.cpp.
// Copyright (c) 2026 Petrus Silva Costa
#include "input_map.hpp"

namespace glintfx {

Rml::Input::KeyIdentifier to_rml_key(Key k) noexcept {
  switch (k) {
    case Key::Up:
      return Rml::Input::KI_UP;
    case Key::Down:
      return Rml::Input::KI_DOWN;
    case Key::Left:
      return Rml::Input::KI_LEFT;
    case Key::Right:
      return Rml::Input::KI_RIGHT;
    case Key::Enter:
      return Rml::Input::KI_RETURN;
    case Key::Escape:
      return Rml::Input::KI_ESCAPE;
    case Key::Tab:
      return Rml::Input::KI_TAB;
    case Key::Space:
      return Rml::Input::KI_SPACE;
    case Key::Backspace:
      return Rml::Input::KI_BACK;
    case Key::Delete:
      return Rml::Input::KI_DELETE;
    case Key::Home:
      return Rml::Input::KI_HOME;
    case Key::End:
      return Rml::Input::KI_END;
    case Key::PageUp:
      return Rml::Input::KI_PRIOR;
    case Key::PageDown:
      return Rml::Input::KI_NEXT;
    default:
      return Rml::Input::KI_UNKNOWN;
  }
}

int to_rml_mods(int mods) noexcept {
  int result = 0;
  if (mods & Mod_Shift) result |= Rml::Input::KM_SHIFT; // EN: Shift. PT: Shift.
  if (mods & Mod_Ctrl) result |= Rml::Input::KM_CTRL;   // EN: Ctrl.  PT: Ctrl.
  if (mods & Mod_Alt) result |= Rml::Input::KM_ALT;     // EN: Alt.   PT: Alt.
  return result;
}

} // namespace glintfx
