// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glfw_event_translate.hpp (A1, framework-2D) -- exercises the GLFW->
//     UiEvent translation functions directly, WITHOUT creating a window/GL context (no
//     glfwInit() call anywhere in this file; only #define constants from <GLFW/glfw3.h> are
//     used, pulled in transitively by the header under test). This is the "dente" against the
//     class of bug the house discipline calls out explicitly (memória
//     feedback_cadencia_releases_consumidor): a conversion/mapping formula shipped without a
//     delta test that exercises it is exactly the class of bug that only shows up in
//     production. Two entries the plan singled out by name get a DEDICATED, isolated
//     assertion each: the scroll sign inversion, and the cursor fb/win HiDPI scale (which Xvfb
//     itself can never exercise, since Xvfb always reports framebuffer size == window size --
//     see glfw_translate_cursor_pos's doc-comment).
//     Mutation-testability note for the adversarial reviewer: removing EITHER negation in
//     glfw_translate_scroll, or the fb/win multiplication in glfw_translate_cursor_pos, MUST
//     turn one of the assertions below red -- that is the whole point of this file existing.
// PT: Teste unit puro para glfw_event_translate.hpp (A1, framework-2D) -- exercita as funções
//     de tradução GLFW->UiEvent diretamente, SEM criar janela/contexto GL (nenhuma chamada a
//     glfwInit() em lugar nenhum deste arquivo; só constantes #define de <GLFW/glfw3.h> são
//     usadas, puxadas transitivamente pelo header sob teste). Este é o "dente" contra a classe
//     de bug que a disciplina da casa nomeia explicitamente (memória
//     feedback_cadencia_releases_consumidor): uma fórmula de conversão/mapeamento entregue sem
//     um teste de delta que a exercite é exatamente a classe de bug que só aparece em produção.
//     Dois itens que o plano nomeou explicitamente ganham UMA asserção DEDICADA e isolada cada:
//     a inversão de sinal do scroll, e a escala fb/win de HiDPI do cursor (que o próprio Xvfb
//     nunca consegue exercitar, já que o Xvfb sempre reporta tamanho de framebuffer == tamanho
//     de janela -- ver o doc-comment de glfw_translate_cursor_pos).
//     Nota de mutation-testability pro reviewer adversarial: remover QUALQUER UMA das negações
//     em glfw_translate_scroll, ou a multiplicação fb/win em glfw_translate_cursor_pos, TEM que
//     deixar vermelha alguma das asserções abaixo -- esse é o ponto inteiro deste arquivo
//     existir.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/glfw_event_translate.hpp"
#include <cstdio>
#include <cmath>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

bool close(float a, float b, float tol = 0.01f) { return std::fabs(a - b) <= tol; }

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: glfw_translate_key -- exhaustive map, including KP_ENTER, plus unknown-key discard.
  // PT: glfw_translate_key -- mapa exaustivo, incluindo KP_ENTER, mais descarte de tecla
  //     desconhecida.
  // ---------------------------------------------------------------------------
  {
    Key k;
    check(glfw_translate_key(GLFW_KEY_UP, k)        && k == Key::Up,        "GLFW_KEY_UP -> Key::Up");
    check(glfw_translate_key(GLFW_KEY_DOWN, k)      && k == Key::Down,      "GLFW_KEY_DOWN -> Key::Down");
    check(glfw_translate_key(GLFW_KEY_LEFT, k)      && k == Key::Left,      "GLFW_KEY_LEFT -> Key::Left");
    check(glfw_translate_key(GLFW_KEY_RIGHT, k)     && k == Key::Right,     "GLFW_KEY_RIGHT -> Key::Right");
    check(glfw_translate_key(GLFW_KEY_ENTER, k)     && k == Key::Enter,     "GLFW_KEY_ENTER -> Key::Enter");
    check(glfw_translate_key(GLFW_KEY_KP_ENTER, k)  && k == Key::Enter,     "GLFW_KEY_KP_ENTER -> Key::Enter");
    check(glfw_translate_key(GLFW_KEY_ESCAPE, k)    && k == Key::Escape,    "GLFW_KEY_ESCAPE -> Key::Escape");
    check(glfw_translate_key(GLFW_KEY_TAB, k)       && k == Key::Tab,       "GLFW_KEY_TAB -> Key::Tab");
    check(glfw_translate_key(GLFW_KEY_SPACE, k)     && k == Key::Space,     "GLFW_KEY_SPACE -> Key::Space");
    check(glfw_translate_key(GLFW_KEY_BACKSPACE, k) && k == Key::Backspace, "GLFW_KEY_BACKSPACE -> Key::Backspace");
    check(glfw_translate_key(GLFW_KEY_DELETE, k)    && k == Key::Delete,    "GLFW_KEY_DELETE -> Key::Delete");
    check(glfw_translate_key(GLFW_KEY_HOME, k)      && k == Key::Home,      "GLFW_KEY_HOME -> Key::Home");
    check(glfw_translate_key(GLFW_KEY_END, k)       && k == Key::End,       "GLFW_KEY_END -> Key::End");
    check(glfw_translate_key(GLFW_KEY_PAGE_UP, k)   && k == Key::PageUp,    "GLFW_KEY_PAGE_UP -> Key::PageUp");
    check(glfw_translate_key(GLFW_KEY_PAGE_DOWN, k) && k == Key::PageDown,  "GLFW_KEY_PAGE_DOWN -> Key::PageDown");

    // EN: A letter key (outside the nav-oriented subset) must be DISCARDED (false), not
    //     mapped to some default -- proves the "do not synthesise an event" contract.
    // PT: Uma tecla de letra (fora do subconjunto orientado a nav) precisa ser DESCARTADA
    //     (false), não mapeada para algum default -- prova o contrato "não sintetizar evento".
    k = Key::Up;  // EN: sentinel -- must stay untouched. PT: sentinela -- precisa ficar intocado.
    check(!glfw_translate_key(GLFW_KEY_A, k), "GLFW_KEY_A must be discarded (outside nav subset)");
    check(k == Key::Up, "glfw_translate_key must not touch out_key on a discarded key");
  }

  // ---------------------------------------------------------------------------
  // EN: glfw_translate_action_pressed -- PRESS and REPEAT both -> true; RELEASE -> false.
  // PT: glfw_translate_action_pressed -- PRESS e REPEAT ambos -> true; RELEASE -> false.
  // ---------------------------------------------------------------------------
  check(glfw_translate_action_pressed(GLFW_PRESS)   == true,  "GLFW_PRESS -> pressed=true");
  check(glfw_translate_action_pressed(GLFW_REPEAT)  == true,  "GLFW_REPEAT -> pressed=true (held-key auto-repeat)");
  check(glfw_translate_action_pressed(GLFW_RELEASE) == false, "GLFW_RELEASE -> pressed=false");

  // ---------------------------------------------------------------------------
  // EN: glfw_translate_mods -- explicit per-bit mapping, including combinations.
  // PT: glfw_translate_mods -- mapeamento explícito bit-a-bit, incluindo combinações.
  // ---------------------------------------------------------------------------
  check(glfw_translate_mods(0) == Mod_None, "no GLFW mods -> Mod_None");
  check(glfw_translate_mods(GLFW_MOD_SHIFT) == Mod_Shift, "GLFW_MOD_SHIFT -> Mod_Shift");
  check(glfw_translate_mods(GLFW_MOD_CONTROL) == Mod_Ctrl, "GLFW_MOD_CONTROL -> Mod_Ctrl");
  check(glfw_translate_mods(GLFW_MOD_ALT) == Mod_Alt, "GLFW_MOD_ALT -> Mod_Alt");
  check(glfw_translate_mods(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT) ==
            (Mod_Shift | Mod_Ctrl | Mod_Alt),
        "Shift+Ctrl+Alt combination maps to the OR of all three glintfx::Mod bits");
  // EN: GLFW_MOD_SUPER/CAPS_LOCK/NUM_LOCK (bits not in glintfx::Mod) must not leak through as
  //     spurious bits.
  // PT: GLFW_MOD_SUPER/CAPS_LOCK/NUM_LOCK (bits que não estão em glintfx::Mod) não podem vazar
  //     como bits espúrios.
  check(glfw_translate_mods(GLFW_MOD_SUPER) == Mod_None, "GLFW_MOD_SUPER alone -> Mod_None (not part of glintfx::Mod)");

  // ---------------------------------------------------------------------------
  // EN: glfw_translate_mouse_button -- LEFT/RIGHT/MIDDLE -> 0/1/2; anything else discarded.
  // PT: glfw_translate_mouse_button -- LEFT/RIGHT/MIDDLE -> 0/1/2; qualquer outro descartado.
  // ---------------------------------------------------------------------------
  {
    int b = -1;
    check(glfw_translate_mouse_button(GLFW_MOUSE_BUTTON_LEFT, b)   && b == 0, "LEFT -> button 0");
    check(glfw_translate_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, b)  && b == 1, "RIGHT -> button 1");
    check(glfw_translate_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, b) && b == 2, "MIDDLE -> button 2");
    b = 42;
    check(!glfw_translate_mouse_button(GLFW_MOUSE_BUTTON_4, b), "side button 4 must be discarded");
    check(b == 42, "glfw_translate_mouse_button must not touch out_button on a discarded button");
  }

  // ---------------------------------------------------------------------------
  // EN: glfw_translate_cursor_pos -- THE HiDPI CASE XVFB CANNOT EXERCISE (fb/win ratio != 1).
  //     A 2x-scaled display: 1280x720 window backed by a 2560x1440 framebuffer. GLFW screen
  //     coords (100, 50) must scale to framebuffer pixels (200, 100).
  // PT: glfw_translate_cursor_pos -- O CASO HIDPI QUE O XVFB NÃO CONSEGUE EXERCITAR (razão
  //     fb/win != 1). Um display escalado 2x: janela 1280x720 sustentada por framebuffer
  //     2560x1440. Coordenadas de tela GLFW (100, 50) precisam escalar pra pixels de
  //     framebuffer (200, 100).
  // ---------------------------------------------------------------------------
  {
    float x = -1.f, y = -1.f;
    glfw_translate_cursor_pos(100.0, 50.0, /*win*/ 1280, 720, /*fb*/ 2560, 1440, x, y);
    check(close(x, 200.0f), "HiDPI 2x: x 100 -> 200 (fb/win scale)");
    check(close(y, 100.0f), "HiDPI 2x: y 50 -> 100 (fb/win scale)");

    // EN: 1:1 (Xvfb's actual case, win == fb) must be a pure passthrough (no accidental scale).
    // PT: 1:1 (o caso real do Xvfb, win == fb) precisa ser repasse puro (sem escala acidental).
    glfw_translate_cursor_pos(42.0, 17.0, 300, 200, 300, 200, x, y);
    check(close(x, 42.0f) && close(y, 17.0f), "1:1 scale (Xvfb's actual case) is a pure passthrough");

    // EN: A non-integer scale (1.5x, e.g. a 150% display scale factor) must also round-trip
    //     correctly -- not just clean powers of two.
    // PT: Uma escala não-inteira (1.5x, ex.: fator de escala de display 150%) também precisa
    //     bater certo -- não só potências de dois limpas.
    glfw_translate_cursor_pos(100.0, 100.0, 200, 200, 300, 300, x, y);
    check(close(x, 150.0f) && close(y, 150.0f), "1.5x non-integer scale");

    // EN: Degenerate window size (0, transiently possible between a resize and the next
    //     framebuffer-size callback) must fall back to 1:1 rather than dividing by zero /
    //     producing NaN or inf.
    // PT: Tamanho de janela degenerado (0, transitoriamente possível entre um resize e o
    //     próximo callback de framebuffer-size) precisa cair de volta para 1:1 em vez de
    //     dividir por zero / produzir NaN ou inf.
    glfw_translate_cursor_pos(10.0, 10.0, 0, 0, 1280, 720, x, y);
    check(std::isfinite(x) && std::isfinite(y), "win_w/win_h <= 0 must not produce NaN/inf");
    check(close(x, 10.0f) && close(y, 10.0f), "win_w/win_h <= 0 falls back to 1:1 scale");
  }

  // ---------------------------------------------------------------------------
  // EN: glfw_translate_scroll -- SIGN INVERSION ON BOTH AXES (the other item the plan named
  //     explicitly). Confirmed against the pinned RmlUi 6.3 GLFW backend's own y-negation --
  //     see glfw_translate_scroll's doc-comment in the header under test for the full citation
  //     and the x-by-symmetry rationale.
  // PT: glfw_translate_scroll -- INVERSÃO DE SINAL NOS DOIS EIXOS (o outro item que o plano
  //     nomeou explicitamente). Confirmado contra a própria negação de y do backend GLFW
  //     pinado do RmlUi 6.3 -- ver o doc-comment de glfw_translate_scroll no header sob teste
  //     pra citação completa e a racional de x-por-simetria.
  // ---------------------------------------------------------------------------
  {
    float x = 0.f, y = 0.f;
    glfw_translate_scroll(1.0, 1.0, x, y);
    check(close(x, -1.0f), "scroll: positive xoffset -> negative UiEvent x (sign inverted)");
    check(close(y, -1.0f), "scroll: positive yoffset -> negative UiEvent y (sign inverted)");

    glfw_translate_scroll(-2.5, -3.5, x, y);
    check(close(x, 2.5f), "scroll: negative xoffset -> positive UiEvent x");
    check(close(y, 3.5f), "scroll: negative yoffset -> positive UiEvent y");

    glfw_translate_scroll(0.0, 0.0, x, y);
    check(close(x, 0.0f) && close(y, 0.0f), "scroll: zero offset -> zero delta (no spurious sign flip of zero)");
  }

  // ---------------------------------------------------------------------------
  // EN: glfw_encode_utf8 -- ASCII, 2-byte, 3-byte, 4-byte ranges, plus the two invalid-input
  //     rejections (surrogate range, > U+10FFFF).
  // PT: glfw_encode_utf8 -- faixas ASCII, 2-byte, 3-byte, 4-byte, mais as duas rejeições de
  //     entrada inválida (faixa de surrogate, > U+10FFFF).
  // ---------------------------------------------------------------------------
  {
    char buf[5];
    int  n;

    n = glfw_encode_utf8('A', buf);  // U+0041
    check(n == 1 && buf[0] == 'A' && buf[1] == '\0', "ASCII 'A' -> 1 byte");

    n = glfw_encode_utf8(0x00E3, buf);  // 'ã' (LATIN SMALL LETTER A WITH TILDE) -- pt-br
    check(n == 2 &&
          static_cast<unsigned char>(buf[0]) == 0xC3 &&
          static_cast<unsigned char>(buf[1]) == 0xA3 &&
          buf[2] == '\0',
          "U+00E3 'a-til' -> 2-byte UTF-8 (pt-br accented Latin-1 Supplement)");

    n = glfw_encode_utf8(0x4E2D, buf);  // '中' -- 3-byte range
    check(n == 3 &&
          static_cast<unsigned char>(buf[0]) == 0xE4 &&
          static_cast<unsigned char>(buf[1]) == 0xB8 &&
          static_cast<unsigned char>(buf[2]) == 0xAD &&
          buf[3] == '\0',
          "U+4E2D -> 3-byte UTF-8");

    n = glfw_encode_utf8(0x1F600, buf);  // grinning-face emoji -- 4-byte range
    check(n == 4 &&
          static_cast<unsigned char>(buf[0]) == 0xF0 &&
          static_cast<unsigned char>(buf[1]) == 0x9F &&
          static_cast<unsigned char>(buf[2]) == 0x98 &&
          static_cast<unsigned char>(buf[3]) == 0x80 &&
          buf[4] == '\0',
          "U+1F600 (emoji) -> 4-byte UTF-8");

    n = glfw_encode_utf8(0xD800, buf);  // UTF-16 surrogate range: invalid Unicode scalar value
    check(n == 0, "surrogate codepoint 0xD800 must be rejected (n == 0)");

    n = glfw_encode_utf8(0x110000, buf);  // one past the Unicode ceiling 0x10FFFF
    check(n == 0, "codepoint 0x110000 (past the Unicode ceiling) must be rejected (n == 0)");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "glfw_event_translate_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("glfw_event_translate_sanity: PASS");
  return 0;
}
