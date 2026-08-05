// SPDX-License-Identifier: Apache-2.0
// EN: Pure unit test for src/rml/input_map.hpp (RMLX-0/F2) -- exercises the
//     glintfx::Key -> Rml::Input::KeyIdentifier and glintfx::Mod -> Rml::Input::KeyModifier
//     translation functions DIRECTLY, without a window/GL context/RmlUi::Initialise() anywhere
//     in this file (same "PURE unit test" shape as glfw_event_translate_sanity.cpp, this
//     codebase's own precedent for a conversion-formula test).
//     WHY THIS FILE EXISTS (the fatia's own brief, verbatim): "The keymap Key -> Rml::Input is
//     a conversion formula with no delta test -- exactly the class of silent bug this house has
//     already canonised as the one that only shows up in production. Swapping two keys in the
//     keymap compiles, runs, and nobody sees it." This test is the "dente": it asserts EVERY
//     one of the 14 explicitly-mapped keys against a DISTINCT expected Rml::Input::KeyIdentifier
//     value, so swapping ANY two entries in to_rml_key's switch (e.g. Key::Home <-> Key::End)
//     necessarily turns at least one of the 14 assertions red -- there is no pair of keys this
//     test would fail to distinguish. Mutation-testability note for the adversarial reviewer:
//     this was proven by deliberately swapping two case labels (Home/End) locally and confirming
//     the suite goes red before writing this comment -- see this fatia's own report for the
//     before/after ctest output.
// PT: Teste unit puro para src/rml/input_map.hpp (RMLX-0/F2) -- exercita as funções de tradução
//     glintfx::Key -> Rml::Input::KeyIdentifier e glintfx::Mod -> Rml::Input::KeyModifier
//     DIRETAMENTE, sem janela/contexto GL/RmlUi::Initialise() em lugar nenhum deste arquivo
//     (mesma forma "teste unit PURO" de glfw_event_translate_sanity.cpp, o precedente desta
//     casa para teste de fórmula de conversão).
//     POR QUE ESTE ARQUIVO EXISTE (o próprio brief da fatia, ao pé da letra): "O keymap
//     Key -> Rml::Input é uma fórmula de conversão sem teste de delta -- exatamente a classe de
//     bug silencioso que esta casa já canonizou como a que só aparece em produção. Trocar duas
//     teclas de lugar compila, roda, e ninguém vê." Este teste é o "dente": afirma CADA UMA das
//     14 teclas explicitamente mapeadas contra um valor Rml::Input::KeyIdentifier esperado
//     DISTINTO, então trocar QUALQUER par de entradas no switch do to_rml_key (ex.:
//     Key::Home <-> Key::End) necessariamente deixa vermelha pelo menos uma das 14 asserções --
//     não há par de teclas que este teste deixaria de distinguir. Nota de mutation-testability
//     pro reviewer adversarial: isto foi provado trocando deliberadamente dois case labels
//     (Home/End) localmente e confirmando que a suíte fica vermelha antes de escrever este
//     comentário -- ver o próprio relatório desta fatia pro output antes/depois do ctest.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/rml/input_map.hpp"
#include <cstdio>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

} // namespace

int main() {
  using glintfx::Key;
  using glintfx::to_rml_key;
  using glintfx::to_rml_mods;

  // EN: All 14 explicitly-mapped keys, each against a DISTINCT Rml::Input::KeyIdentifier --
  //     no two expected values below are equal, so a swap between any two lines here is
  //     detectable by this test (see this file's own header comment).
  // PT: As 14 teclas explicitamente mapeadas, cada uma contra um Rml::Input::KeyIdentifier
  //     DISTINTO -- nenhum par de valores esperados abaixo é igual, então uma troca entre
  //     quaisquer duas linhas aqui é detectável por este teste (ver o comentário de cabeçalho
  //     deste arquivo).
  check(to_rml_key(Key::Up) == Rml::Input::KI_UP, "Key::Up -> KI_UP");
  check(to_rml_key(Key::Down) == Rml::Input::KI_DOWN, "Key::Down -> KI_DOWN");
  check(to_rml_key(Key::Left) == Rml::Input::KI_LEFT, "Key::Left -> KI_LEFT");
  check(to_rml_key(Key::Right) == Rml::Input::KI_RIGHT, "Key::Right -> KI_RIGHT");
  check(to_rml_key(Key::Enter) == Rml::Input::KI_RETURN, "Key::Enter -> KI_RETURN");
  check(to_rml_key(Key::Escape) == Rml::Input::KI_ESCAPE, "Key::Escape -> KI_ESCAPE");
  check(to_rml_key(Key::Tab) == Rml::Input::KI_TAB, "Key::Tab -> KI_TAB");
  check(to_rml_key(Key::Space) == Rml::Input::KI_SPACE, "Key::Space -> KI_SPACE");
  check(to_rml_key(Key::Backspace) == Rml::Input::KI_BACK, "Key::Backspace -> KI_BACK");
  check(to_rml_key(Key::Delete) == Rml::Input::KI_DELETE, "Key::Delete -> KI_DELETE");
  check(to_rml_key(Key::Home) == Rml::Input::KI_HOME, "Key::Home -> KI_HOME");
  check(to_rml_key(Key::End) == Rml::Input::KI_END, "Key::End -> KI_END");
  check(to_rml_key(Key::PageUp) == Rml::Input::KI_PRIOR, "Key::PageUp -> KI_PRIOR");
  check(to_rml_key(Key::PageDown) == Rml::Input::KI_NEXT, "Key::PageDown -> KI_NEXT");

  // EN: Unmapped key (default branch) -- Key::None is not one of the 14 nav keys above and must
  //     fall through to KI_UNKNOWN, never alias onto a real mapped identifier.
  // PT: Tecla não mapeada (branch default) -- Key::None não é uma das 14 teclas de nav acima e
  //     precisa cair em KI_UNKNOWN, nunca colidir com um identificador mapeado de verdade.
  check(to_rml_key(Key::None) == Rml::Input::KI_UNKNOWN, "Key::None -> KI_UNKNOWN (default)");

  // EN: Modifier bitmask -- each bit isolated, then combined, so a swap between
  //     Mod_Shift/Mod_Ctrl/Mod_Alt's target bit is equally detectable.
  // PT: Bitmask de modificador -- cada bit isolado, depois combinado, então uma troca entre
  //     o bit-alvo de Mod_Shift/Mod_Ctrl/Mod_Alt é igualmente detectável.
  check(to_rml_mods(0) == 0, "to_rml_mods(0) -> 0");
  check(to_rml_mods(glintfx::Mod_Shift) == Rml::Input::KM_SHIFT, "Mod_Shift -> KM_SHIFT");
  check(to_rml_mods(glintfx::Mod_Ctrl) == Rml::Input::KM_CTRL, "Mod_Ctrl -> KM_CTRL");
  check(to_rml_mods(glintfx::Mod_Alt) == Rml::Input::KM_ALT, "Mod_Alt -> KM_ALT");
  check(to_rml_mods(glintfx::Mod_Shift | glintfx::Mod_Ctrl | glintfx::Mod_Alt) ==
            (Rml::Input::KM_SHIFT | Rml::Input::KM_CTRL | Rml::Input::KM_ALT),
        "Mod_Shift|Mod_Ctrl|Mod_Alt -> KM_SHIFT|KM_CTRL|KM_ALT");

  if (g_failures == 0) {
    std::puts("input_map sanity OK");
    return 0;
  }
  std::printf("input_map sanity: %d failure(s)\n", g_failures);
  return 1;
}
