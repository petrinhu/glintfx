// SPDX-License-Identifier: MPL-2.0
// EN: Verifies UiLayer::set_click_callback(): (A) direct id on the clicked element; (B) click
//     on a child WITHOUT an id bubbles up to report the nearest ancestor id ("panel"); (C) click
//     on a subtree with NO id anywhere reports "" (never a crash, never silently dropped);
//     (D) reentrancy (AUD-TEC-3) -- a callback that calls set_click_callback(...) on itself
//     from inside its own invocation must not use-after-free (ClickEventListener::ProcessEvent
//     copies the functor locally before invoking it; run this test under ASan to catch a
//     regression of that guard).
//     A click is simulated as MouseMove (to hover/set mouse_position) + MouseButtonDown + Up at
//     the same point -- RmlUi's own click detection (down+up on the same element == click).
// PT: Verifica UiLayer::set_click_callback(): (A) id direto no elemento clicado; (B) clique num
//     filho SEM id sobe até reportar o ancestral mais próximo com id ("panel"); (C) clique numa
//     subárvore SEM id em lugar nenhum reporta "" (nunca crash, nunca descartado silenciosamente);
//     (D) reentrância (AUD-TEC-3) -- um callback que chama set_click_callback(...) em si mesmo
//     de dentro da própria invocação não pode dar use-after-free (ClickEventListener::
//     ProcessEvent copia o functor localmente antes de invocá-lo; rodar este teste sob ASan
//     para capturar uma regressão dessa guarda).
//     Um clique é simulado como MouseMove (hover/mouse_position) + MouseButtonDown + Up no mesmo
//     ponto -- a própria detecção de clique do RmlUi (down+up no mesmo elemento == click).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <string>
#include <vector>

static void click_at(glintfx::UiLayer& ui, float x, float y) {
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = x, .y = y });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = true });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = false });
}

// EN: AUD-PUB-4 (v0.5.0) -- synthesizes a non-primary (right=1/middle=2) click the same way
//     click_at() synthesizes a left one: MouseMove (hover) + MouseButton down + MouseButton up,
//     same point, given button. Exercises ClickInfoButtonDownListener/UpListener in
//     bootstrap.cpp (Mousedown/Mouseup pair, since RmlUi never dispatches native Click for
//     non-primary buttons).
// PT: AUD-PUB-4 (v0.5.0) -- sintetiza um clique não-primário (direito=1/meio=2) do mesmo jeito
//     que click_at() sintetiza um esquerdo: MouseMove (hover) + MouseButton down + MouseButton
//     up, mesmo ponto, button dado. Exercita ClickInfoButtonDownListener/UpListener em
//     bootstrap.cpp (par Mousedown/Mouseup, já que o RmlUi nunca despacha Click nativo para
//     botões não-primários).
static void button_click_at(glintfx::UiLayer& ui, int button, float x, float y) {
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = x, .y = y });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = button, .pressed = true });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = button, .pressed = false });
}

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("click_cb_host", 300, 200)) { std::puts("FAIL: host create"); return 1; }

  glintfx::UiLayer ui({ .logical_width = 300, .logical_height = 200, .load_gl = true });
  if (!ui.ok()) { std::puts("FAIL: ui attach"); return 2; }

  std::vector<std::string> hits;
  ui.set_click_callback([&hits](const char* id) { hits.emplace_back(id); });

  if (!ui.load("click_scene.rml")) { std::puts("FAIL: load"); return 3; }
  ui.update(); ui.render();

  // (A) direct id
  click_at(ui, 50.f, 30.f);
  // (B) child without id -> bubbles to "panel"
  click_at(ui, 50.f, 80.f);
  // (C) subtree without any id -> ""
  click_at(ui, 50.f, 130.f);

  if (hits.size() != 3) {
    std::fprintf(stderr, "FAIL: expected 3 hits, got %zu\n", hits.size());
    return 4;
  }
  if (hits[0] != "btn_a") { std::fprintf(stderr, "FAIL: hits[0]='%s' expected 'btn_a'\n", hits[0].c_str()); return 5; }
  if (hits[1] != "panel") { std::fprintf(stderr, "FAIL: hits[1]='%s' expected 'panel'\n", hits[1].c_str()); return 6; }
  if (hits[2] != "")      { std::fprintf(stderr, "FAIL: hits[2]='%s' expected ''\n", hits[2].c_str()); return 7; }

  // EN: A click outside any element must NOT invoke the callback (no 4th hit).
  // PT: Um clique fora de qualquer elemento NÃO deve invocar o callback (sem 4º hit).
  click_at(ui, 250.f, 190.f);
  if (hits.size() != 3) { std::fprintf(stderr, "FAIL: click outside body added a hit\n"); return 8; }

  // ---------------------------------------------------------------------------
  // EN: (D) Reentrancy (AUD-TEC-3) -- the first handler replaces itself with a second handler
  //     from INSIDE its own invocation (plausible real pattern: "clicked 'Play' -> swap in the
  //     next screen's handler"). Without the local-copy guard in ClickEventListener::
  //     ProcessEvent, the std::move-assignment inside the nested set_click_callback() call
  //     destroys the std::function target (this very closure, heap-allocated because it captures
  //     3 references) while ProcessEvent is still invoking it through the cb_ pointer. The
  //     `swapped = true;` line AFTER the nested set_click_callback() call below is the actual
  //     tooth: it re-derefs the reference-to-`swapped` stored INSIDE this closure's own
  //     heap storage. With the guard (`auto cb = *cb_` in bootstrap.cpp -- a stack copy survives
  //     reassignment of *cb_), that storage is untouched and the write is safe. Without the
  //     guard, *cb_ was just destroyed by the nested call and this line is a genuine
  //     heap-use-after-free that ASan catches (proven empirically: this test previously passed
  //     identically with and without the guard because nothing was read/written post-reassignment
  //     -- there was no dereference for ASan to catch).
  // PT: (D) Reentrância (AUD-TEC-3) -- o primeiro handler troca a si mesmo por um segundo
  //     handler de DENTRO da própria invocação (padrão real plausível: "clicou em 'Play' ->
  //     troca o handler da tela seguinte"). Sem a guarda de cópia local em
  //     ClickEventListener::ProcessEvent, o std::move-assignment dentro da chamada aninhada a
  //     set_click_callback() destrói o alvo do std::function (esta própria closure, alocada no
  //     heap por capturar 3 referências) enquanto ProcessEvent ainda a invoca via o ponteiro
  //     cb_. A linha `swapped = true;` APÓS a chamada aninhada a set_click_callback() abaixo é
  //     o dente de fato: ela re-derreferencia a referência-a-`swapped` guardada DENTRO do
  //     armazenamento no heap desta própria closure. Com a guarda (`auto cb = *cb_` em
  //     bootstrap.cpp -- uma cópia na stack sobrevive à reatribuição de *cb_), esse
  //     armazenamento fica intocado e a escrita é segura. Sem a guarda, *cb_ acabou de ser
  //     destruído pela chamada aninhada e esta linha é um heap-use-after-free real que o ASan
  //     captura (provado empiricamente: este teste antes passava idêntico com e sem a guarda
  //     porque nada era lido/escrito pós-reatribuição -- não havia dereferência para o ASan
  //     capturar).
  // ---------------------------------------------------------------------------
  std::vector<std::string> reentrant_hits;
  bool swapped = false;
  ui.set_click_callback([&ui, &reentrant_hits, &swapped](const char* id) {
    reentrant_hits.emplace_back(id);
    if (!swapped) {
      swapped = true;
      ui.set_click_callback([&reentrant_hits](const char* id2) {
        reentrant_hits.emplace_back(std::string("second:") + id2);
      });
      // EN: Tooth of this test -- see comment block above. Reads AND writes `swapped` through
      //     the reference stored inside THIS closure, after the closure may have been freed.
      // PT: Dente deste teste -- ver bloco de comentário acima. Lê E escreve `swapped` através
      //     da referência guardada DENTRO desta closure, depois que ela pode ter sido liberada.
      swapped = true;
    }
  });

  click_at(ui, 50.f, 30.f);  // 1st handler fires, swaps itself out mid-invocation
  click_at(ui, 50.f, 30.f);  // 2nd handler (installed above) now fires

  if (reentrant_hits.size() != 2) {
    std::fprintf(stderr, "FAIL: reentrancy expected 2 hits, got %zu\n", reentrant_hits.size());
    return 9;
  }
  if (reentrant_hits[0] != "btn_a") {
    std::fprintf(stderr, "FAIL: reentrant_hits[0]='%s' expected 'btn_a'\n", reentrant_hits[0].c_str());
    return 10;
  }
  if (reentrant_hits[1] != "second:btn_a") {
    std::fprintf(stderr, "FAIL: reentrant_hits[1]='%s' expected 'second:btn_a'\n",
                 reentrant_hits[1].c_str());
    return 11;
  }

  // ---------------------------------------------------------------------------
  // EN: (E) AUD-PUB-4 (v0.5.0) -- set_click_info_callback: parallel channel, does NOT replace
  //     the id-only callback above (both fire independently on the same click, proven by (F)
  //     below re-checking `hits` still grows).
  // PT: (E) AUD-PUB-4 (v0.5.0) -- set_click_info_callback: canal paralelo, NÃO substitui o
  //     callback só-id acima (ambos disparam independentemente no mesmo clique, provado pelo
  //     (F) abaixo reconferindo que `hits` continua crescendo).
  // ---------------------------------------------------------------------------
  std::vector<glintfx::ClickInfo> info_hits;
  ui.set_click_info_callback([&info_hits](const glintfx::ClickInfo& info) {
    info_hits.push_back(info);
  });

  // (E1) Single click on btn_a: id="btn_a", button=0 (left -- the only button RmlUi's Click
  // event ever carries, see ClickInfo's doc-comment), coords close to the click point (50,30 --
  // "close" because mouse_x/mouse_y report where the button was RELEASED, not exactly the
  // MouseMove target; both are inside btn_a's box either way), double_click=false.
  click_at(ui, 50.f, 30.f);
  if (info_hits.size() != 1) {
    std::fprintf(stderr, "FAIL(E1): expected 1 info hit, got %zu\n", info_hits.size());
    return 12;
  }
  if (std::string(info_hits[0].id) != "btn_a") {
    std::fprintf(stderr, "FAIL(E1): id='%s' expected 'btn_a'\n", info_hits[0].id);
    return 13;
  }
  if (info_hits[0].button != 0) {
    std::fprintf(stderr, "FAIL(E1): button=%d expected 0\n", info_hits[0].button);
    return 14;
  }
  if (info_hits[0].double_click) {
    std::fprintf(stderr, "FAIL(E1): double_click=true on a single click\n");
    return 15;
  }
  // EN: Coordinates must land inside btn_a's box (see click_scene.rcss) -- a loose bound
  //     (not an exact-pixel match) because RmlUi reports the release point, and window-space
  //     == content-local space here (no set_viewport(x,y,w,h,target_h) letterbox offset was
  //     configured by this test, so UiLayer's translation is a no-op (+0)).
  // PT: As coordenadas devem cair dentro da caixa do btn_a (ver click_scene.rcss) -- limite
  //     frouxo (não exact-pixel) porque o RmlUi reporta o ponto de soltura, e espaço-janela
  //     == espaço local de conteúdo aqui (nenhum offset de letterbox
  //     set_viewport(x,y,w,h,target_h) foi configurado por este teste, então a tradução do
  //     UiLayer é um no-op (+0)).
  if (info_hits[0].x < 0.f || info_hits[0].x > 300.f || info_hits[0].y < 0.f || info_hits[0].y > 200.f) {
    std::fprintf(stderr, "FAIL(E1): coords (%.1f, %.1f) out of viewport bounds\n",
                 info_hits[0].x, info_hits[0].y);
    return 16;
  }

  // (E2) One more click_at() on the SAME spot, immediately after E1 (within RmlUi's 0.5s / 3dp
  // thresholds, both trivially satisfied -- no sleep, same exact point) -- E1's click already
  // armed RmlUi's internal last_click_element/last_click_time (Context.cpp:660-673), so THIS
  // click_at's mousedown is itself the "second click" of the pair: it fires Dblclick
  // (double_click=true) BEFORE its own mouseup fires the paired Click (double_click=false) --
  // 2 more info_hits from this single click_at call. A second click_at() right after that
  // consumes the RESET state (Dblclick's dispatch clears last_click_element/time,
  // Context.cpp:666-667) as a fresh, non-double single click -- 1 more info_hit. Total: 4 hits
  // after E1+E2 combined (1 from E1 + 3 from E2's two click_at calls), verified by grepping the
  // pinned RmlUi source: ProcessMouseButtonDown dispatches Dblclick on mousedown when
  // active==last_click_element within the time/distance window and immediately resets that
  // state; ProcessMouseButtonUp unconditionally dispatches Click when button_index==0 and
  // active==hover, regardless of the double-click state (mouseup always fires Click, mousedown
  // conditionally ALSO fires Dblclick -- they are independent RmlUi events, not one replacing
  // the other, hence 2 events for the "second click" of a double-click).
  // PT: (E2) Mais um click_at() no MESMO ponto, logo após E1 (dentro dos limiares de 0.5s/3dp
  // do RmlUi, ambos trivialmente satisfeitos -- sem sleep, mesmo ponto exato) -- o clique de E1
  // já armou o last_click_element/last_click_time interno do RmlUi (Context.cpp:660-673),
  // então o mousedown DESTE click_at já É o "segundo clique" do par: dispara Dblclick
  // (double_click=true) ANTES do próprio mouseup disparar o Click pareado (double_click=false)
  // -- 2 info_hits a mais desta única chamada de click_at. Um segundo click_at() logo depois
  // consome o estado RESETADO (o despacho do Dblclick limpa last_click_element/time,
  // Context.cpp:666-667) como um clique único novo, não-duplo -- mais 1 info_hit. Total: 4 hits
  // após E1+E2 combinados (1 de E1 + 3 das duas chamadas click_at de E2), verificado grepando o
  // source pinado do RmlUi: ProcessMouseButtonDown dispara Dblclick no mousedown quando
  // active==last_click_element dentro da janela de tempo/distância e reseta esse estado
  // imediatamente; ProcessMouseButtonUp incondicionalmente dispara Click quando
  // button_index==0 e active==hover, independente do estado de duplo-clique (mouseup sempre
  // dispara Click, mousedown condicionalmente TAMBÉM dispara Dblclick -- são eventos RmlUi
  // independentes, um não substitui o outro, por isso 2 eventos para o "segundo clique" de um
  // duplo-clique).
  click_at(ui, 50.f, 30.f);
  click_at(ui, 50.f, 30.f);
  if (info_hits.size() != 4) {
    std::fprintf(stderr, "FAIL(E2): expected 4 info hits after a double-click sequence, got %zu\n",
                 info_hits.size());
    return 17;
  }
  if (!info_hits[1].double_click) {
    std::fprintf(stderr, "FAIL(E2): info_hits[1] (Dblclick, from E2's 1st mousedown) reported double_click=false\n");
    return 18;
  }
  if (info_hits[2].double_click) {
    std::fprintf(stderr, "FAIL(E2): info_hits[2] (Click, from E2's 1st mouseup) reported double_click=true\n");
    return 19;
  }
  if (info_hits[3].double_click) {
    std::fprintf(stderr, "FAIL(E2): info_hits[3] (Click, from E2's 2nd click_at -- state reset after the Dblclick) reported double_click=true\n");
    return 20;
  }

  // ---------------------------------------------------------------------------
  // EN: (F) Non-regression: the id-only callback (A/B/C/D above) must still be alive and firing
  //     after set_click_info_callback was registered -- proves the two channels are independent
  //     (neither set_* call clobbers the other's storage). `hits` from the reentrancy test (D)
  //     ended at 2; each of the 3 clicks in (E1/E2) above adds one more "second:btn_a" hit via
  //     the still-installed 2nd reentrant handler.
  // PT: (F) Não-regressão: o callback só-id (A/B/C/D acima) precisa continuar vivo e disparando
  //     após set_click_info_callback ter sido registrado -- prova que os dois canais são
  //     independentes (nenhuma chamada set_* atropela o armazenamento da outra). `hits` do
  //     teste de reentrância (D) terminou em 2; cada um dos 3 cliques em (E1/E2) acima soma
  //     mais um hit "second:btn_a" via o 2º handler reentrante ainda instalado.
  // ---------------------------------------------------------------------------
  if (reentrant_hits.size() != 5) {
    std::fprintf(stderr,
                 "FAIL(F): expected 5 reentrant_hits (id-only callback still firing), got %zu\n",
                 reentrant_hits.size());
    return 21;
  }

  // ---------------------------------------------------------------------------
  // EN: (G) AUD-PUB-4 gap closure (v0.5.0) -- right(1)/middle(2) ClickInfo::button is now
  //     FUNCTIONAL, synthesized from Mousedown+Mouseup (RmlUi has no native Click for
  //     non-primary buttons; see click_info.hpp's "SOURCE" doc-comment). `hits.size()` (the
  //     id-only channel, fed only by native EventId::Click) must NOT grow from these -- proves
  //     Mousedown/Mouseup handling is isolated to the new listener pair and does not leak into
  //     ClickEventListener.
  // PT: (G) Fechamento de gap AUD-PUB-4 (v0.5.0) -- ClickInfo::button direito(1)/meio(2) agora é
  //     FUNCIONAL, sintetizado de Mousedown+Mouseup (o RmlUi não tem Click nativo para botões
  //     não-primários; ver o doc-comment "ORIGEM" de click_info.hpp). `hits.size()` (o canal
  //     só-id, alimentado só por EventId::Click nativo) NÃO pode crescer com estes -- prova que
  //     o tratamento de Mousedown/Mouseup fica isolado no novo par de listeners e não vaza para
  //     o ClickEventListener.
  // ---------------------------------------------------------------------------
  const size_t hits_before_button_synthesis = hits.size();

  button_click_at(ui, 1, 50.f, 30.f);  // right-click on btn_a
  if (info_hits.size() != 5) {
    std::fprintf(stderr, "FAIL(G1): expected 5 info hits after right-click, got %zu\n", info_hits.size());
    return 22;
  }
  if (info_hits[4].button != 1) {
    std::fprintf(stderr, "FAIL(G1): button=%d expected 1 (right)\n", info_hits[4].button);
    return 23;
  }
  if (std::string(info_hits[4].id) != "btn_a") {
    std::fprintf(stderr, "FAIL(G1): id='%s' expected 'btn_a'\n", info_hits[4].id);
    return 24;
  }
  if (info_hits[4].double_click) {
    std::fprintf(stderr, "FAIL(G1): double_click=true on a right-click (no native Dblclick for non-primary buttons)\n");
    return 25;
  }

  button_click_at(ui, 2, 50.f, 30.f);  // middle-click on btn_a
  if (info_hits.size() != 6) {
    std::fprintf(stderr, "FAIL(G2): expected 6 info hits after middle-click, got %zu\n", info_hits.size());
    return 26;
  }
  if (info_hits[5].button != 2) {
    std::fprintf(stderr, "FAIL(G2): button=%d expected 2 (middle)\n", info_hits[5].button);
    return 27;
  }

  if (hits.size() != hits_before_button_synthesis) {
    std::fprintf(stderr,
                 "FAIL(G3): id-only channel grew from right/middle clicks (%zu -> %zu) -- "
                 "Mousedown/Mouseup leaked into ClickEventListener\n",
                 hits_before_button_synthesis, hits.size());
    return 28;
  }

  // EN: (G4) Non-regression -- left click still reports button=0 through the SAME
  //     set_click_info_callback channel after the right/middle listeners fired above.
  // PT: (G4) Não-regressão -- clique esquerdo continua reportando button=0 pelo MESMO canal
  //     set_click_info_callback depois dos listeners de direito/meio terem disparado acima.
  click_at(ui, 50.f, 30.f);
  if (info_hits.back().button != 0) {
    std::fprintf(stderr, "FAIL(G4): left click after right/middle reported button=%d, expected 0\n",
                 info_hits.back().button);
    return 29;
  }

  // ---------------------------------------------------------------------------
  // EN: (H) Drag-off -- Mousedown{button=1} on btn_a, Mouseup{button=1} on "panel" (a DIFFERENT
  //     element) must NOT fire click_info_cb (mirrors RmlUi's own left-click rule: a press that
  //     is released over a different element is not a click). Proves
  //     ClickInfoButtonUpListener's id-match guard, not just its happy path above.
  // PT: (H) Arrastar-para-fora -- Mousedown{button=1} no btn_a, Mouseup{button=1} no "panel" (um
  //     elemento DIFERENTE) NÃO pode disparar click_info_cb (espelha a própria regra de clique
  //     esquerdo do RmlUi: uma pressão solta sobre um elemento diferente não é um clique). Prova
  //     a guarda de correspondência de id do ClickInfoButtonUpListener, não só o caminho feliz
  //     acima.
  // ---------------------------------------------------------------------------
  const size_t info_hits_before_drag_off = info_hits.size();
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 50.f, .y = 30.f });   // hover btn_a
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 1, .pressed = true });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 50.f, .y = 80.f });   // hover panel
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 1, .pressed = false });
  if (info_hits.size() != info_hits_before_drag_off) {
    std::fprintf(stderr,
                 "FAIL(H): drag-off (down on btn_a, up on panel) fired a click, got %zu info hits "
                 "(expected no change from %zu)\n",
                 info_hits.size(), info_hits_before_drag_off);
    return 30;
  }

  std::puts("click_callback_sanity: PASS");
  return 0;
}
