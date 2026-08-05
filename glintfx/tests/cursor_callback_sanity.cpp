// SPDX-License-Identifier: Apache-2.0
// EN: Verifies UiLayer::set_cursor_callback() (AUD-PUB-6g). RmlUi's Context caches the LAST
//     dispatched cursor name (Context::cursor_name, default "") and calls
//     SystemInterface::SetMouseCursor() ONLY when the newly-hovered element's computed `cursor`
//     property differs from that cache (Context::UpdateHoverChain, RmlUi/Source/Core/
//     Context.cpp) -- confirmed by reading the pinned source, not assumed. This test is built
//     around that "fires on CHANGE only" semantics, not "fires on every MouseMove":
//       (A) entering cursor_el (cursor: pointer) from a fresh UiLayer (internal cache starts at
//           "") dispatches exactly once, name == "pointer".
//       (B) a SECOND MouseMove that stays INSIDE cursor_el (different point, same element, same
//           computed cursor) must NOT redispatch -- the armadilha the CTO named explicitly.
//       (C) moving OUT to "plain" (no cursor property, computed "") dispatches again, name=="".
//       (D) a further MouseMove that stays inside "plain" must NOT redispatch either (same
//           armadilha, mirrored on the empty-string side).
//       (E) moving back INTO cursor_el dispatches a THIRD time, name == "pointer" again --
//           proves the dedup is a live comparison against the CURRENT cache, not a one-shot
//           latch that stops firing after the first transition.
//       (F) reentrancy (AUD-TEC-3) -- a callback that calls set_cursor_callback(...) on itself
//           from inside its own invocation must not use-after-free (SystemClock::SetMouseCursor
//           copies the functor locally before invoking it; same discipline as
//           ClickEventListener::ProcessEvent in bootstrap.cpp -- run under ASan to catch a
//           regression of that guard).
// PT: Verifica UiLayer::set_cursor_callback() (AUD-PUB-6g). O Context do RmlUi cacheia o ÚLTIMO
//     nome de cursor despachado (Context::cursor_name, default "") e só chama
//     SystemInterface::SetMouseCursor() quando a propriedade `cursor` computada do elemento
//     recém-hovered difere desse cache (Context::UpdateHoverChain, RmlUi/Source/Core/
//     Context.cpp) -- confirmado lendo o source pinado, não suposto. Este teste é desenhado em
//     torno dessa semântica "só dispara na MUDANÇA", não "dispara em todo MouseMove":
//       (A) entrar em cursor_el (cursor: pointer) a partir de um UiLayer fresco (cache interno
//           começa em "") dispara exatamente uma vez, nome == "pointer".
//       (B) um SEGUNDO MouseMove que permanece DENTRO de cursor_el (ponto diferente, mesmo
//           elemento, mesmo cursor computado) NÃO pode redisparar -- a armadilha que o CTO
//           nomeou explicitamente.
//       (C) mover para FORA, para "plain" (sem propriedade cursor, computado ""), dispara de
//           novo, nome=="".
//       (D) um MouseMove adicional que permanece dentro de "plain" também NÃO pode redisparar
//           (mesma armadilha, espelhada no lado da string vazia).
//       (E) mover de volta PARA cursor_el dispara uma TERCEIRA vez, nome == "pointer" de novo --
//           prova que o dedup é uma comparação viva contra o cache CORRENTE, não uma trava de
//           um-tiro-só que para de disparar após a primeira transição.
//       (F) reentrância (AUD-TEC-3) -- um callback que chama set_cursor_callback(...) em si
//           mesmo de dentro da própria invocação não pode dar use-after-free
//           (SystemClock::SetMouseCursor copia o functor localmente antes de invocá-lo; mesma
//           disciplina de ClickEventListener::ProcessEvent em bootstrap.cpp -- rodar sob ASan
//           para capturar uma regressão dessa guarda).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <string>
#include <vector>

static void move_to(glintfx::UiLayer& ui, float x, float y) {
  ui.process_event({.type = glintfx::UiEvent::Type::MouseMove, .x = x, .y = y});
}

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("cursor_cb_host", 300, 200)) {
    std::puts("FAIL: host create");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = 300, .logical_height = 200});
  if (!ui.ok()) {
    std::puts("FAIL: ui attach");
    return 2;
  }

  std::vector<std::string> cursors;
  ui.set_cursor_callback([&cursors](const char* name) { cursors.emplace_back(name); });

  if (!ui.load("cursor_scene.rml")) {
    std::puts("FAIL: load");
    return 3;
  }
  ui.update();
  ui.render();

  // (A) enter cursor_el -- "" (RmlUi Context default) -> "pointer".
  move_to(ui, 50.f, 30.f);
  if (cursors.size() != 1) {
    std::fprintf(stderr, "FAIL(A): expected 1 dispatch after entering cursor_el, got %zu\n",
                 cursors.size());
    return 4;
  }
  if (cursors[0] != "pointer") {
    std::fprintf(stderr, "FAIL(A): cursors[0]='%s' expected 'pointer'\n", cursors[0].c_str());
    return 5;
  }

  // (B) THE armadilha -- a second MouseMove to a DIFFERENT point still inside cursor_el's box
  //     must NOT redispatch (same computed cursor, RmlUi's own cache dedupes).
  move_to(ui, 70.f, 55.f);
  if (cursors.size() != 1) {
    std::fprintf(stderr,
                 "FAIL(B): MouseMove within the SAME element redispatched -- expected 1, got %zu "
                 "(RmlUi only calls SetMouseCursor on a CHANGE, not per MouseMove)\n",
                 cursors.size());
    return 6;
  }

  // (C) leave to "plain" (no cursor property, computed "") -- "pointer" -> "".
  move_to(ui, 50.f, 110.f);
  if (cursors.size() != 2) {
    std::fprintf(stderr, "FAIL(C): expected 2 dispatches after leaving to plain, got %zu\n",
                 cursors.size());
    return 7;
  }
  if (cursors[1] != std::string("")) {
    std::fprintf(stderr, "FAIL(C): cursors[1]='%s' expected ''\n", cursors[1].c_str());
    return 8;
  }

  // (D) armadilha mirrored on the "" side -- staying inside "plain" must not redispatch.
  move_to(ui, 60.f, 130.f);
  if (cursors.size() != 2) {
    std::fprintf(stderr,
                 "FAIL(D): MouseMove within plain (still computed \"\") redispatched -- expected "
                 "2, got %zu\n",
                 cursors.size());
    return 9;
  }

  // (E) back into cursor_el -- dedup is a live comparison, not a one-shot latch.
  move_to(ui, 50.f, 30.f);
  if (cursors.size() != 3) {
    std::fprintf(stderr, "FAIL(E): expected 3 dispatches after re-entering cursor_el, got %zu\n",
                 cursors.size());
    return 10;
  }
  if (cursors[2] != "pointer") {
    std::fprintf(stderr, "FAIL(E): cursors[2]='%s' expected 'pointer'\n", cursors[2].c_str());
    return 11;
  }

  // ---------------------------------------------------------------------------
  // (F) Reentrancy (AUD-TEC-3) -- the handler replaces itself with a second handler from INSIDE
  //     its own invocation. Without SystemClock::SetMouseCursor's local-copy guard, the
  //     std::move-assignment inside the nested set_cursor_callback() call destroys the
  //     std::function target (this very closure, heap-allocated because it captures 2
  //     references) while SetMouseCursor is still invoking it -- the `swapped = true;` line
  //     AFTER the nested call re-derefs a reference stored INSIDE that same closure's own heap
  //     storage, a genuine heap-use-after-free ASan catches without the guard (same tooth as
  //     click_callback_sanity.cpp's reentrancy leg (D)).
  // ---------------------------------------------------------------------------
  std::vector<std::string> reentrant;
  bool swapped = false;
  ui.set_cursor_callback([&ui, &reentrant, &swapped](const char* name) {
    reentrant.emplace_back(name);
    if (!swapped) {
      swapped = true;
      ui.set_cursor_callback(
          [&reentrant](const char* n2) { reentrant.emplace_back(std::string("second:") + n2); });
      swapped = true; // tooth: re-derefs `swapped` through this closure's own storage
    }
  });

  move_to(ui, 50.f, 110.f); // -> plain, "pointer" -> "" : 1st handler fires, swaps itself out
  move_to(ui, 50.f, 30.f);  // -> cursor_el, "" -> "pointer" : 2nd handler (installed above) fires

  if (reentrant.size() != 2) {
    std::fprintf(stderr, "FAIL(F): reentrancy expected 2 hits, got %zu\n", reentrant.size());
    return 12;
  }
  if (reentrant[0] != std::string("")) {
    std::fprintf(stderr, "FAIL(F): reentrant[0]='%s' expected ''\n", reentrant[0].c_str());
    return 13;
  }
  if (reentrant[1] != "second:pointer") {
    std::fprintf(stderr, "FAIL(F): reentrant[1]='%s' expected 'second:pointer'\n",
                 reentrant[1].c_str());
    return 14;
  }

  std::puts("cursor_callback_sanity: PASS");
  return 0;
}
