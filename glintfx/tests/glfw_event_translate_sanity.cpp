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

    // EN: HOSTIN-1 (Onda 2, v0.19.0) note -- a letter key (GLFW_KEY_A) USED TO be the "outside
    //     the mapped subset, must be discarded" example here, back when glfw_translate_key only
    //     understood the 14 nav-oriented keys. HOSTIN-1 EXTENDS the mapped subset to include
    //     letters/digits/F-keys/modifiers/etc. (see the dedicated HOSTIN-1 block below), so
    //     GLFW_KEY_A now translates successfully -- the "must be discarded" negative case moved
    //     to GLFW_KEY_UNKNOWN there, the input that is STILL genuinely unmapped after this wave.
    // PT: Nota HOSTIN-1 (Onda 2, v0.19.0) -- uma tecla de letra (GLFW_KEY_A) COSTUMAVA ser o
    //     exemplo de "fora do subconjunto mapeado, precisa ser descartada" aqui, de quando
    //     glfw_translate_key só entendia as 14 teclas orientadas a nav. O HOSTIN-1 ESTENDE o
    //     subconjunto mapeado para incluir letras/dígitos/F-teclas/modificadores/etc. (ver o
    //     bloco dedicado HOSTIN-1 abaixo), então GLFW_KEY_A agora traduz com sucesso -- o caso
    //     negativo "precisa ser descartado" mudou para GLFW_KEY_UNKNOWN lá, o input que AINDA é
    //     genuinamente não-mapeado depois desta onda.
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

  // ---------------------------------------------------------------------------
  // EN: HOSTIN-1 (Onda 2, v0.19.0, D1) -- glfw_translate_key's ~92-entry extension. Every
  //     named GLFW key added this wave must round-trip to its enumerator; a handful of
  //     representative spot-checks per category (not literally all 92 -- the append-only
  //     static_asserts in ui_event.hpp already pin the numeric layout, and the switch itself is
  //     mechanical/exhaustive by construction) plus the two negative cases that matter: an
  //     unmapped GLFW_KEY_UNKNOWN-shaped input stays discarded, and KP_ENTER keeps folding into
  //     Key::Enter (unchanged since before this wave).
  // PT: HOSTIN-1 (Onda 2, v0.19.0, D1) -- extensão de ~92 entradas do glfw_translate_key. Toda
  //     tecla GLFW nomeada adicionada nesta onda precisa fazer round-trip pro próprio
  //     enumerador; um punhado de checagens representativas por categoria (não literalmente as
  //     92 -- os static_asserts append-only em ui_event.hpp já fixam o layout numérico, e o
  //     próprio switch é mecânico/exaustivo por construção) mais os dois casos negativos que
  //     importam: um input com formato GLFW_KEY_UNKNOWN não-mapeado continua descartado, e
  //     KP_ENTER continua dobrando em Key::Enter (inalterado desde antes desta onda).
  // ---------------------------------------------------------------------------
  {
    Key k;
    check(glfw_translate_key(GLFW_KEY_A, k) && k == Key::A, "GLFW_KEY_A -> Key::A");
    check(glfw_translate_key(GLFW_KEY_W, k) && k == Key::W, "GLFW_KEY_W -> Key::W (WASD)");
    check(glfw_translate_key(GLFW_KEY_Z, k) && k == Key::Z, "GLFW_KEY_Z -> Key::Z (last letter)");
    check(glfw_translate_key(GLFW_KEY_0, k) && k == Key::Digit0, "GLFW_KEY_0 -> Key::Digit0");
    check(glfw_translate_key(GLFW_KEY_9, k) && k == Key::Digit9, "GLFW_KEY_9 -> Key::Digit9");
    check(glfw_translate_key(GLFW_KEY_F1, k) && k == Key::F1, "GLFW_KEY_F1 -> Key::F1");
    check(glfw_translate_key(GLFW_KEY_F12, k) && k == Key::F12, "GLFW_KEY_F12 -> Key::F12");
    check(glfw_translate_key(GLFW_KEY_LEFT_SHIFT, k) && k == Key::LeftShift, "GLFW_KEY_LEFT_SHIFT -> Key::LeftShift");
    check(glfw_translate_key(GLFW_KEY_RIGHT_SHIFT, k) && k == Key::RightShift, "GLFW_KEY_RIGHT_SHIFT -> Key::RightShift");
    check(glfw_translate_key(GLFW_KEY_LEFT_CONTROL, k) && k == Key::LeftControl, "GLFW_KEY_LEFT_CONTROL -> Key::LeftControl");
    check(glfw_translate_key(GLFW_KEY_RIGHT_CONTROL, k) && k == Key::RightControl, "GLFW_KEY_RIGHT_CONTROL -> Key::RightControl");
    check(glfw_translate_key(GLFW_KEY_LEFT_ALT, k) && k == Key::LeftAlt, "GLFW_KEY_LEFT_ALT -> Key::LeftAlt");
    check(glfw_translate_key(GLFW_KEY_RIGHT_ALT, k) && k == Key::RightAlt, "GLFW_KEY_RIGHT_ALT -> Key::RightAlt");
    check(glfw_translate_key(GLFW_KEY_LEFT_SUPER, k) && k == Key::LeftSuper, "GLFW_KEY_LEFT_SUPER -> Key::LeftSuper");
    check(glfw_translate_key(GLFW_KEY_RIGHT_SUPER, k) && k == Key::RightSuper, "GLFW_KEY_RIGHT_SUPER -> Key::RightSuper");
    check(glfw_translate_key(GLFW_KEY_INSERT, k) && k == Key::Insert, "GLFW_KEY_INSERT -> Key::Insert");
    check(glfw_translate_key(GLFW_KEY_CAPS_LOCK, k) && k == Key::CapsLock, "GLFW_KEY_CAPS_LOCK -> Key::CapsLock");
    check(glfw_translate_key(GLFW_KEY_SCROLL_LOCK, k) && k == Key::ScrollLock, "GLFW_KEY_SCROLL_LOCK -> Key::ScrollLock");
    check(glfw_translate_key(GLFW_KEY_NUM_LOCK, k) && k == Key::NumLock, "GLFW_KEY_NUM_LOCK -> Key::NumLock");
    check(glfw_translate_key(GLFW_KEY_PRINT_SCREEN, k) && k == Key::PrintScreen, "GLFW_KEY_PRINT_SCREEN -> Key::PrintScreen");
    check(glfw_translate_key(GLFW_KEY_PAUSE, k) && k == Key::Pause, "GLFW_KEY_PAUSE -> Key::Pause");
    check(glfw_translate_key(GLFW_KEY_MENU, k) && k == Key::Menu, "GLFW_KEY_MENU -> Key::Menu");
    check(glfw_translate_key(GLFW_KEY_APOSTROPHE, k) && k == Key::Apostrophe, "GLFW_KEY_APOSTROPHE -> Key::Apostrophe");
    check(glfw_translate_key(GLFW_KEY_COMMA, k) && k == Key::Comma, "GLFW_KEY_COMMA -> Key::Comma");
    check(glfw_translate_key(GLFW_KEY_MINUS, k) && k == Key::Minus, "GLFW_KEY_MINUS -> Key::Minus");
    check(glfw_translate_key(GLFW_KEY_PERIOD, k) && k == Key::Period, "GLFW_KEY_PERIOD -> Key::Period");
    check(glfw_translate_key(GLFW_KEY_SLASH, k) && k == Key::Slash, "GLFW_KEY_SLASH -> Key::Slash");
    check(glfw_translate_key(GLFW_KEY_SEMICOLON, k) && k == Key::Semicolon, "GLFW_KEY_SEMICOLON -> Key::Semicolon (ABNT2 'c cedilha' position)");
    check(glfw_translate_key(GLFW_KEY_EQUAL, k) && k == Key::Equal, "GLFW_KEY_EQUAL -> Key::Equal");
    check(glfw_translate_key(GLFW_KEY_LEFT_BRACKET, k) && k == Key::LeftBracket, "GLFW_KEY_LEFT_BRACKET -> Key::LeftBracket");
    check(glfw_translate_key(GLFW_KEY_BACKSLASH, k) && k == Key::Backslash, "GLFW_KEY_BACKSLASH -> Key::Backslash");
    check(glfw_translate_key(GLFW_KEY_RIGHT_BRACKET, k) && k == Key::RightBracket, "GLFW_KEY_RIGHT_BRACKET -> Key::RightBracket");
    check(glfw_translate_key(GLFW_KEY_GRAVE_ACCENT, k) && k == Key::GraveAccent, "GLFW_KEY_GRAVE_ACCENT -> Key::GraveAccent");
    check(glfw_translate_key(GLFW_KEY_WORLD_1, k) && k == Key::World1, "GLFW_KEY_WORLD_1 -> Key::World1 (ABNT2/ISO extra key)");
    check(glfw_translate_key(GLFW_KEY_WORLD_2, k) && k == Key::World2, "GLFW_KEY_WORLD_2 -> Key::World2");
    check(glfw_translate_key(GLFW_KEY_KP_0, k) && k == Key::Kp0, "GLFW_KEY_KP_0 -> Key::Kp0");
    check(glfw_translate_key(GLFW_KEY_KP_9, k) && k == Key::Kp9, "GLFW_KEY_KP_9 -> Key::Kp9");
    check(glfw_translate_key(GLFW_KEY_KP_DECIMAL, k) && k == Key::KpDecimal, "GLFW_KEY_KP_DECIMAL -> Key::KpDecimal");
    check(glfw_translate_key(GLFW_KEY_KP_DIVIDE, k) && k == Key::KpDivide, "GLFW_KEY_KP_DIVIDE -> Key::KpDivide");
    check(glfw_translate_key(GLFW_KEY_KP_MULTIPLY, k) && k == Key::KpMultiply, "GLFW_KEY_KP_MULTIPLY -> Key::KpMultiply");
    check(glfw_translate_key(GLFW_KEY_KP_SUBTRACT, k) && k == Key::KpSubtract, "GLFW_KEY_KP_SUBTRACT -> Key::KpSubtract");
    check(glfw_translate_key(GLFW_KEY_KP_ADD, k) && k == Key::KpAdd, "GLFW_KEY_KP_ADD -> Key::KpAdd");
    check(glfw_translate_key(GLFW_KEY_KP_EQUAL, k) && k == Key::KpEqual, "GLFW_KEY_KP_EQUAL -> Key::KpEqual");

    // EN: KP_ENTER must STILL fold into Key::Enter, not a separate Kp-prefixed enumerator --
    //     this pre-existing behaviour (v0.4.0) must survive the HOSTIN-1 enum growth unchanged.
    // PT: KP_ENTER precisa CONTINUAR dobrando em Key::Enter, não um enumerador separado com
    //     prefixo Kp -- este comportamento pré-existente (v0.4.0) precisa sobreviver ao
    //     crescimento do enum do HOSTIN-1 sem mudança.
    check(glfw_translate_key(GLFW_KEY_KP_ENTER, k) && k == Key::Enter, "GLFW_KEY_KP_ENTER still folds into Key::Enter (unchanged)");

    // EN: An input with no matching case (any unassigned integer GLFW itself would report as
    //     GLFW_KEY_UNKNOWN) must still be discarded, exactly like the pre-HOSTIN-1 letter-key
    //     rejection above -- the switch growing to ~106 cases does not turn the default branch
    //     into dead code.
    // PT: Um input sem case correspondente (qualquer inteiro não atribuído que o próprio GLFW
    //     reportaria como GLFW_KEY_UNKNOWN) ainda precisa ser descartado, exatamente como a
    //     rejeição de tecla-letra pré-HOSTIN-1 acima -- o switch crescer para ~106 casos não
    //     transforma o ramo default em código morto.
    k = Key::A; // EN: sentinel. PT: sentinela.
    check(!glfw_translate_key(GLFW_KEY_UNKNOWN, k), "GLFW_KEY_UNKNOWN must be discarded");
    check(k == Key::A, "glfw_translate_key must not touch out_key on GLFW_KEY_UNKNOWN");
  }

  // ---------------------------------------------------------------------------
  // EN: glfw_key_is_ui_forwardable (D10) -- exactly the pre-existing 14 nav-oriented keys are
  //     forwarded to the UI/RmlUi route; none of the ~92 HOSTIN-1 additions are. Mutation-
  //     testability note: flipping this predicate to `true` for ANY key below (e.g. Key::A) --
  //     or to `false` for any of the pre-existing 14 -- must turn one of these assertions red.
  // PT: glfw_key_is_ui_forwardable (D10) -- exatamente as 14 teclas pré-existentes orientadas a
  //     navegação são encaminhadas à rota de UI/RmlUi; nenhuma das ~92 adições do HOSTIN-1 é.
  //     Nota de mutation-testability: virar este predicado para `true` em QUALQUER tecla abaixo
  //     (ex.: Key::A) -- ou para `false` em qualquer uma das 14 pré-existentes -- precisa
  //     deixar alguma destas asserções vermelha.
  // ---------------------------------------------------------------------------
  check(glfw_key_is_ui_forwardable(Key::Up) == true, "Key::Up is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Down) == true, "Key::Down is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Left) == true, "Key::Left is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Right) == true, "Key::Right is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Enter) == true, "Key::Enter is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Escape) == true, "Key::Escape is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Tab) == true, "Key::Tab is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Space) == true, "Key::Space is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Backspace) == true, "Key::Backspace is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Delete) == true, "Key::Delete is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::Home) == true, "Key::Home is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::End) == true, "Key::End is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::PageUp) == true, "Key::PageUp is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::PageDown) == true, "Key::PageDown is UI-forwardable (pre-existing)");
  check(glfw_key_is_ui_forwardable(Key::A) == false, "Key::A is NOT UI-forwardable (HOSTIN-1 addition, D10)");
  check(glfw_key_is_ui_forwardable(Key::W) == false, "Key::W is NOT UI-forwardable (WASD, HOSTIN-1 addition, D10)");
  check(glfw_key_is_ui_forwardable(Key::Digit0) == false, "Key::Digit0 is NOT UI-forwardable (D10)");
  check(glfw_key_is_ui_forwardable(Key::F1) == false, "Key::F1 is NOT UI-forwardable (D10)");
  check(glfw_key_is_ui_forwardable(Key::LeftShift) == false, "Key::LeftShift is NOT UI-forwardable (D10)");
  check(glfw_key_is_ui_forwardable(Key::Kp0) == false, "Key::Kp0 is NOT UI-forwardable (D10)");
  check(glfw_key_is_ui_forwardable(Key::None) == false, "Key::None is NOT UI-forwardable");
  check(glfw_key_is_ui_forwardable(Key::Count) == false, "Key::Count (sentinel) is NOT UI-forwardable");

  // ---------------------------------------------------------------------------
  // EN: glfw_decide_window_close (HOSTIN-4, D6) -- pure decision seam for the user-initiated
  //     close veto. Mutation-testability note: inverting either branch below must turn one of
  //     these red.
  // PT: glfw_decide_window_close (HOSTIN-4, D6) -- seam de decisão pura pro veto de close
  //     iniciado pelo usuário. Nota de mutation-testability: inverter qualquer um dos ramos
  //     abaixo precisa deixar alguma destas vermelha.
  // ---------------------------------------------------------------------------
  check(glfw_decide_window_close(false, false) == true, "no callback registered -> close proceeds (default GLFW behaviour)");
  check(glfw_decide_window_close(false, true) == true, "no callback registered -> close proceeds regardless of the unused 2nd arg");
  check(glfw_decide_window_close(true, true) == true, "callback returned true -> close proceeds");
  check(glfw_decide_window_close(true, false) == false, "callback returned false -> close is VETOED");

  if (g_failures > 0) {
    std::fprintf(stderr, "glfw_event_translate_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("glfw_event_translate_sanity: PASS");
  return 0;
}
