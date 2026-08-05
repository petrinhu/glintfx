// SPDX-License-Identifier: Apache-2.0
// EN: AUD-PUB-6g-TEARDOWN -- regression test for the App/SystemInterfaceGlfwDedup TWIN of the
//     heap-use-after-free cursor_callback_sanity.cpp's reentrancy leg (F) first caught (via
//     ASan, CI run 31014883530, job "sanitize (self-hosted / GLFW=ON)"): RmlUi's
//     Context::~Context() -> UnloadAllDocuments() -> UnloadDocument() -> UpdateHoverChain()
//     (RmlUi/Source/Core/Context.cpp, pinned source, confirmed by reading it) dispatches ONE
//     FINAL SystemInterface::SetMouseCursor() call as the hover chain clears during the
//     Context's OWN destruction, whenever the computed cursor differs from
//     Context::cursor_name's last-dispatched value (it almost always does -- hovering nothing
//     computes ""). SystemInterfaceGlfwDedup::SetMouseCursor (src/system_glfw_dedup.hpp)
//     forwards that call straight to whatever callback a consumer last installed via
//     App::set_cursor_callback() -- so a callback that captures a reference to a LOCAL variable
//     declared AFTER the App (and therefore destroyed BEFORE it, C++'s own reverse-declaration-
//     order rule) is invoked with that reference already dangling.
//     This is the App-mode twin of cursor_callback_sanity.cpp's own reentrancy leg (F) --
//     confirmed present in src/system_glfw_dedup.hpp's SetMouseCursor override, same shape as
//     SystemClock::SetMouseCursor (src/rml/system_clock.hpp) -- and the SAME shape is what the
//     AUD-PUB-6g-TEARDOWN fix closes on BOTH sides (App::Impl::~Impl() here,
//     UiLayer::Impl::~Impl() there): the consumer's cursor callback is disarmed BEFORE the
//     Engine (and therefore the RmlUi Context) is torn down, so this final dispatch never
//     reaches a dangling reference in the first place.
//     Sequence proving the hazard would fire without the fix: (1) install a callback capturing
//     `&last_cursor` by reference; (2) move the mouse into `cursor_el` (`cursor: pointer`) so
//     RmlUi's Context caches "pointer" as the last-dispatched name -- a value that DIFFERS from
//     the "" the hover chain recomputes to once the document unloads; (3) let `app` and
//     `last_cursor` go out of scope. `last_cursor` is declared AFTER `app`, so it is destroyed
//     BEFORE `app`'s destructor runs (reverse-declaration order) -- if App::Impl still forwarded
//     the teardown-time SetMouseCursor("") call to the still-installed callback, the callback
//     would write into `last_cursor` through a dangling reference. Under ASan (GLINTFX_SANITIZE
//     ON), that use-after-free/use-after-scope is a deterministic abort; under a plain build it
//     is a probabilistic heap corruption depending on allocator/stack reuse -- exactly what made
//     cursor_callback_sanity.cpp intermittent in CI (own font engine + sanitize jobs failed,
//     GitHub-hosted vs. self-hosted disagreed) and consistently green on a developer machine.
//     No explicit assertion is needed to PROVE the fix: with the callback disarmed at teardown
//     time, the final SetMouseCursor("") dispatch is a safe no-op and this test simply exits 0
//     -- the proof is "runs clean under ASan", the same class of proof the AUD-TEC-3 reentrancy
//     legs in click_callback_sanity.cpp/cursor_callback_sanity.cpp already rely on for their own
//     use-after-free guards.
// PT: AUD-PUB-6g-TEARDOWN -- teste de regressão para o GÊMEO no App/SystemInterfaceGlfwDedup do
//     heap-use-after-free que a perna de reentrância (F) do cursor_callback_sanity.cpp
//     descobriu primeiro (via ASan, run de CI 31014883530, job "sanitize (self-hosted /
//     GLFW=ON)"): o Context::~Context() do RmlUi -> UnloadAllDocuments() -> UnloadDocument() ->
//     UpdateHoverChain() (RmlUi/Source/Core/Context.cpp, source pinado, confirmado lendo-o)
//     despacha UMA ÚLTIMA chamada SystemInterface::SetMouseCursor() enquanto a hover chain
//     limpa durante a PRÓPRIA destruição do Context, sempre que o cursor computado difere do
//     último valor despachado em Context::cursor_name (quase sempre difere -- hover em nada
//     computa ""). O SystemInterfaceGlfwDedup::SetMouseCursor (src/system_glfw_dedup.hpp)
//     repassa essa chamada direto para o que quer que um consumidor tenha instalado por último
//     via App::set_cursor_callback() -- então um callback que captura uma referência a uma
//     LOCAL declarada DEPOIS do App (e portanto destruída ANTES dele, a própria regra de
//     ordem-reversa-de-declaração do C++) é invocado com essa referência já pendurada.
//     Este é o gêmeo modo-App da própria perna de reentrância (F) do cursor_callback_sanity.cpp
//     -- confirmado presente no override SetMouseCursor de src/system_glfw_dedup.hpp, mesma
//     forma de SystemClock::SetMouseCursor (src/rml/system_clock.hpp) -- e é a MESMA forma que
//     o fix AUD-PUB-6g-TEARDOWN fecha nos DOIS lados (App::Impl::~Impl() aqui,
//     UiLayer::Impl::~Impl() lá): o callback de cursor do consumidor é desarmado ANTES do
//     Engine (e portanto do Context do RmlUi) ser desmontado, então este despacho final nunca
//     alcança uma referência pendurada, pra começo de conversa.
//     Sequência que provaria a armadilha sem o fix: (1) instala um callback capturando
//     `&last_cursor` por referência; (2) move o mouse pra dentro de `cursor_el`
//     (`cursor: pointer`) pra o Context do RmlUi cachear "pointer" como último nome despachado
//     -- um valor que DIFERE do "" pro qual a hover chain recomputa assim que o documento
//     descarrega; (3) deixa `app` e `last_cursor` saírem de escopo. `last_cursor` é declarada
//     DEPOIS de `app`, então é destruída ANTES do destrutor de `app` rodar (ordem reversa de
//     declaração) -- se o App::Impl ainda repassasse a chamada SetMouseCursor("") de teardown
//     ao callback ainda instalado, o callback escreveria em `last_cursor` através de uma
//     referência pendurada. Sob ASan (GLINTFX_SANITIZE ON), esse use-after-free/
//     use-after-scope é um abort determinístico; sob um build comum é uma corrupção de heap
//     probabilística dependente de reuso de alocador/pilha -- exatamente o que tornava o
//     cursor_callback_sanity.cpp intermitente no CI (jobs de motor-de-fonte-próprio e sanitize
//     falharam, GitHub-hosted vs. self-hosted discordaram) e consistentemente verde numa
//     máquina de desenvolvedor. Nenhuma asserção explícita é necessária pra PROVAR o fix: com o
//     callback desarmado no momento do teardown, o despacho final SetMouseCursor("") é um no-op
//     seguro e este teste simplesmente sai com 0 -- a prova é "roda limpo sob ASan", a mesma
//     classe de prova de que as pernas de reentrância AUD-TEC-3 em
//     click_callback_sanity.cpp/cursor_callback_sanity.cpp já dependem pras próprias guardas de
//     use-after-free.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <string>

int main() {
  glintfx::App app({.title = "app_cursor_teardown_sanity", .width = 300, .height = 200});
  if (!app.ok()) {
    std::puts("FAIL: app ok() false");
    return 1;
  }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); cursor_scene.rml
  //     is copied once to CMAKE_CURRENT_BINARY_DIR by tests/CMakeLists.txt, addressed here with
  //     the "tests/" prefix -- same pattern as app_cursor_callback_smoke.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo);
  //     cursor_scene.rml é copiado uma vez para CMAKE_CURRENT_BINARY_DIR por
  //     tests/CMakeLists.txt, endereçado aqui com o prefixo "tests/" -- mesmo padrão de
  //     app_cursor_callback_smoke.
  if (!app.load("tests/cursor_scene.rml")) {
    std::puts("FAIL: load");
    return 2;
  }
  app.update();
  app.render();

  // EN: `last_cursor` declared AFTER `app` -- destroyed BEFORE `app` at scope exit (C++'s own
  //     reverse-declaration-order rule), the exact lifetime shape the AUD-PUB-6g-TEARDOWN fix
  //     must survive. Captured BY REFERENCE, same as every callback in this library's own
  //     AUD-TEC-3-documented usage pattern -- this is not adversarial input, it is the ordinary
  //     shape a consumer's own callback takes.
  // PT: `last_cursor` declarada DEPOIS de `app` -- destruída ANTES de `app` na saída de escopo
  //     (a própria regra de ordem-reversa-de-declaração do C++), exatamente a forma de vida que
  //     o fix AUD-PUB-6g-TEARDOWN precisa sobreviver. Capturada POR REFERÊNCIA, igual a todo
  //     callback do próprio padrão de uso documentado AUD-TEC-3 desta biblioteca -- isto não é
  //     entrada adversarial, é a forma comum que o callback de um consumidor assume.
  std::string last_cursor;
  app.set_cursor_callback([&last_cursor](const char* name) { last_cursor = name; });

  // EN: Move into cursor_el ("cursor: pointer") so Context::cursor_name caches "pointer" --
  //     DIFFERENT from the "" the hover chain recomputes to once UnloadAllDocuments() clears it
  //     at teardown, which is what makes the final SetMouseCursor("") dispatch actually happen
  //     (a no-change teardown, like app_cursor_callback_smoke.cpp's, never redispatches and so
  //     never exercised this path).
  // PT: Move pra dentro de cursor_el ("cursor: pointer") pra Context::cursor_name cachear
  //     "pointer" -- DIFERENTE do "" pro qual a hover chain recomputa assim que o
  //     UnloadAllDocuments() a limpa no teardown, que é o que faz o despacho final
  //     SetMouseCursor("") de fato acontecer (um teardown sem mudança, como o do
  //     app_cursor_callback_smoke.cpp, nunca redespacha e portanto nunca exercitou este
  //     caminho).
  app.process_event({.type = glintfx::UiEvent::Type::MouseMove, .x = 50.f, .y = 30.f});
  if (last_cursor != "pointer") {
    std::fprintf(stderr, "FAIL: expected last_cursor=='pointer' after entering cursor_el, got '%s'\n",
                 last_cursor.c_str());
    return 3;
  }

  // EN: app (and its still-installed, &last_cursor-capturing callback) is destroyed HERE, at
  //     scope exit, AFTER last_cursor -- exactly the ordering the AUD-PUB-6g-TEARDOWN fix must
  //     survive. No further assertion needed: reaching this point without an ASan abort (or, on
  //     a plain build, without a crash) IS the proof.
  // PT: app (e o próprio callback ainda instalado, capturando &last_cursor) é destruído AQUI, na
  //     saída de escopo, DEPOIS de last_cursor -- exatamente a ordem que o fix
  //     AUD-PUB-6g-TEARDOWN precisa sobreviver. Nenhuma asserção adicional necessária: chegar
  //     até aqui sem um abort do ASan (ou, num build comum, sem um crash) É A PROVA.
  std::puts("app_cursor_teardown_sanity: PASS");
  return 0;
}
