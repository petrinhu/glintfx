// SPDX-License-Identifier: MPL-2.0
// EN: GLFW window + GL3.3 core context. PT: janela GLFW + contexto GL3.3 core.
#pragma once
#include <functional>
#include <glintfx/ui_event.hpp>
#include <glintfx/window_mode.hpp>  // EN: A4-WINMODES. PT: A4-WINMODES.
#include "input_state.hpp"          // EN: HOSTIN-1, Onda 2. PT: HOSTIN-1, Onda 2.
struct GLFWwindow;
struct GLFWmonitor; // EN: WM-VSYNC -- forward-declared, same pattern as GLFWwindow above.
                    // PT: WM-VSYNC -- forward-declarado, mesmo padrão de GLFWwindow acima.
namespace glintfx {
class WindowGlfw {
public:
  WindowGlfw() = default;
  ~WindowGlfw();
  WindowGlfw(const WindowGlfw&) = delete;
  WindowGlfw& operator=(const WindowGlfw&) = delete;

  // EN: A4-WINMODES -- `req_mode` defaults to Windowed so the ~30 pre-existing embed-mode test
  //     fixtures that only use WindowGlfw as a bare GL-context host (create(title, w, h), no
  //     window-mode concern) keep compiling unchanged; glintfx::App always passes an explicit
  //     mode (app.cpp). See window_mode.hpp for the per-mode semantics and app.hpp's
  //     App::set_window_mode() doc-comment for the full contract this create() call backs.
  // PT: A4-WINMODES -- `mode` tem default Windowed para que os ~30 fixtures de teste
  //     pré-existentes do modo embed que só usam WindowGlfw como host nu de contexto GL
  //     (create(title, w, h), sem preocupação de modo de janela) continuem compilando sem
  //     mudança; o glintfx::App sempre passa um modo explícito (app.cpp). Ver window_mode.hpp
  //     para a semântica por modo e o doc-comment de App::set_window_mode() (app.hpp) para o
  //     contrato completo que este create() sustenta.
  bool create(const char* title, int w, int h, WindowMode req_mode = WindowMode::Windowed);
  void make_current();
  void swap();
  void poll();
  bool should_close() const;
  void size(int& w, int& h) const;
  GLFWwindow* handle() const { return win_; }

  // EN: A4-WINMODES -- runtime mode switch. Returns false (no-op, window untouched) when
  //     win_ is null (create() not called or failed) or `req_mode` is not a valid enumerator (an
  //     out-of-range static_cast) -- same fail-high discipline as every other guard in this
  //     file. Re-requesting the last mode WE DISPATCHED (dispatched_mode_, NOT the live,
  //     possibly WM-laggy mode() query -- see that field's doc-comment for the real-Wayland
  //     race this decision closes) is a safe no-op returning true. See window_mode.hpp for
  //     per-mode semantics and app.hpp's App::set_window_mode() for the full public contract
  //     (this is its direct implementation).
  //     Transition mechanics (D7/D8): leaving Windowed saves the current position+size
  //     (glfwGetWindowPos/Size) so a later return to Windowed restores it; a window that was
  //     NEVER windowed (born Maximized/Fullscreen) restores to AppConfig's construction
  //     width×height at position (64,64) -- documented default, avoids a window materialising
  //     under the panel. Fullscreen(Desktop or Exclusive) -> Maximized restores windowed FIRST
  //     (glfwSetWindowMonitor(..., nullptr, ...)), THEN glfwMaximizeWindow() -- mandatory
  //     order, GLFW does not support maximizing a fullscreen window directly. LATENCY: every
  //     successful call blocks the caller for up to ~150ms before returning
  //     (settle_window_system_request(), bounded, NOT a blind sleep -- see its own doc-comment
  //     for the compositor round-trip this closes); the Fullscreen->Maximized path blocks up to
  //     ~300ms (two requests to settle, one for the restore-to-windowed step, one for the
  //     maximize). This is deliberate for a rare, user-initiated action (options menu, Alt+
  //     Enter), NOT a per-frame call -- reproduced live on a real KDE/Wayland session: without
  //     it, a caller issuing back-to-back set_mode() calls (or this function issuing its own
  //     two internal requests) could race the WM's confirmation and end up with the WRONG final
  //     geometry/state. Wayland: window position is not readable/settable (best-effort
  //     restore, documented limitation).
  // PT: A4-WINMODES -- troca de modo em runtime. Retorna false (no-op, janela intocada) quando
  //     win_ é nulo (create() não chamado ou falhou) ou `req_mode` não é um enumerador válido (um
  //     static_cast fora de faixa) -- mesma disciplina fail-high de toda outra guarda deste
  //     arquivo. Repedir o último modo que NÓS despachamos (dispatched_mode_, NÃO a query mode()
  //     viva, potencialmente atrasada pelo WM -- ver o doc-comment daquele campo pra race real
  //     no Wayland que esta decisão fecha) é um no-op seguro que retorna true. Ver
  //     window_mode.hpp para a semântica por modo e App::set_window_mode() (app.hpp) para o
  //     contrato público completo (este é a implementação direta dele).
  //     Mecânica de transição (D7/D8): sair de Windowed salva a posição+tamanho corrente
  //     (glfwGetWindowPos/Size) para que um retorno posterior a Windowed os restaure; uma
  //     janela que NUNCA foi windowed (nasceu Maximized/Fullscreen) restaura para o
  //     width×height de construção do AppConfig na posição (64,64) -- default documentado,
  //     evita a janela materializar sob o painel. Fullscreen(Desktop ou Exclusive) ->
  //     Maximized restaura windowed PRIMEIRO (glfwSetWindowMonitor(..., nullptr, ...)), DEPOIS
  //     glfwMaximizeWindow() -- ordem obrigatória, o GLFW não suporta maximizar uma janela
  //     fullscreen diretamente. LATÊNCIA: toda chamada bem-sucedida bloqueia o chamador por até
  //     ~150ms antes de retornar (settle_window_system_request(), limitado, NÃO um sleep cego --
  //     ver o próprio doc-comment dele pra ida-e-volta com o compositor que isto fecha); o
  //     caminho Fullscreen->Maximized bloqueia até ~300ms (dois pedidos pra assentar, um pro
  //     passo de restaurar-a-windowed, um pro maximizar). Isto é deliberado pra uma ação rara,
  //     iniciada pelo usuário (menu de opções, Alt+Enter), NÃO uma chamada por-frame --
  //     reproduzido ao vivo numa sessão KDE/Wayland real: sem isto, um chamador emitindo
  //     chamadas set_mode() nas costas uma da outra (ou esta função emitindo os próprios dois
  //     pedidos internos) podia entrar em race com a confirmação do WM e acabar com a
  //     geometria/estado final ERRADO. Wayland: posição de janela não é legível/gravável
  //     (restauração best-effort, limitação documentada).
  bool set_mode(WindowMode req_mode);

  // EN: A4-WINMODES -- LIVE window mode, derived from the window system, NOT an echo of the
  //     last set_mode()/create() request: glfwGetWindowMonitor(win_)!=nullptr means fullscreen
  //     (an internal flag distinguishes Desktop from Exclusive -- GLFW itself does not); else
  //     glfwGetWindowAttrib(win_, GLFW_MAXIMIZED) means Maximized; else Windowed. Returns
  //     Windowed when win_ is null. Headless consequence (documented, not a bug): under Xvfb
  //     (no WM) a Maximized request may not be honoured and this can report Windowed --
  //     tolerated by the headless test suite, proven for real by the manual smoke on a live
  //     KDE/GNOME session (see docs/window-modes.md).
  // PT: A4-WINMODES -- modo de janela VIVO, derivado do sistema de janelas, NÃO um eco do
  //     último pedido set_mode()/create(): glfwGetWindowMonitor(win_)!=nullptr significa
  //     fullscreen (uma flag interna distingue Desktop de Exclusive -- o próprio GLFW não
  //     distingue); senão glfwGetWindowAttrib(win_, GLFW_MAXIMIZED) significa Maximized; senão
  //     Windowed. Retorna Windowed quando win_ é nulo. Consequência headless (documentada, não
  //     é bug): sob Xvfb (sem WM) um pedido Maximized pode não ser honrado e isto pode reportar
  //     Windowed -- tolerado pela suíte headless, provado de verdade pelo smoke manual numa
  //     sessão KDE/GNOME real (ver docs/window-modes.md).
  WindowMode mode() const;

  // EN: WM-VSYNC -- request a swap interval (`glfwSwapInterval`), the real GLFW primitive
  //     `App::set_vsync`/`App::set_swap_interval` (app.hpp) forward to. `interval`: `0` disables
  //     vsync (unlimited swaps), `1` synchronises to the monitor's refresh rate, `n>1`
  //     synchronises to every Nth refresh (half-rate, third-rate, ...). Returns `false` (no-op,
  //     nothing requested to the driver) when `win_` is null (create() not called/failed -- same
  //     fail-high discipline as mode()/set_mode() above) OR `interval` is negative -- a
  //     NEGATIVE interval is GLFW's own signal for adaptive/tearing vsync
  //     (`GLX_EXT_swap_control_tear`/`WGL_EXT_swap_control_tear`), which is EXPLICITLY
  //     out-of-scope for this slice (TODO.md `WM-VSYNC`'s own scope note: no arbitrary
  //     cap/adaptive-vsync machinery here, seed a follow-up if a consumer asks) -- rejecting it
  //     here means a caller can never silently opt into a driver behaviour this API was not
  //     designed to promise. CONTEXT (D-ORDER): `glfwSwapInterval` requires the GL context to be
  //     CURRENT on the calling thread; `create()` above already makes `win_`'s context current
  //     immediately after `glfwCreateWindow` and this class never switches it away for the rest
  //     of the App's lifetime (the same single-context invariant `render()`/`snapshot()`'s direct
  //     `glReadPixels`/`glBindFramebuffer` calls in app.cpp already rely on without
  //     re-asserting it) -- so this call does NOT re-call `glfwMakeContextCurrent` itself; it is
  //     only ever safe to call after a successful `create()`, which the `win_` null-guard above
  //     already enforces. Idempotent: calling with the SAME interval twice (or any interval at
  //     all) is a safe, cheap no-op-if-unchanged request to the driver -- GLFW/the driver own
  //     any actual state, this class keeps none of its own (unlike `dispatched_mode_` for window
  //     mode) because there is no GLFW query to desync from. DRIVER HONESTY: like `set_mode()`
  //     above, this is a REQUEST -- some drivers/compositors clamp or ignore it (documented GLFW
  //     limitation, not a glintfx one); `get_monitor_refresh_hz()` below is a SEPARATE read path,
  //     never an echo of the interval requested here.
  // PT: WM-VSYNC -- pede um intervalo de swap (`glfwSwapInterval`), o primitivo GLFW real que
  //     `App::set_vsync`/`App::set_swap_interval` (app.hpp) encaminham. `interval`: `0` desliga
  //     vsync (swaps ilimitados), `1` sincroniza com a taxa de atualização do monitor, `n>1`
  //     sincroniza a cada N-ésimo refresh (meia-taxa, terça-taxa, ...). Retorna `false` (no-op,
  //     nada pedido ao driver) quando `win_` é nulo (create() não chamado/falhou -- mesma
  //     disciplina fail-high de mode()/set_mode() acima) OU `interval` é negativo -- um
  //     intervalo NEGATIVO é o próprio sinal do GLFW pra vsync adaptativo/com tearing
  //     (`GLX_EXT_swap_control_tear`/`WGL_EXT_swap_control_tear`), que é EXPLICITAMENTE fora de
  //     escopo desta fatia (a própria nota de escopo do `WM-VSYNC` no TODO.md: nenhuma máquina
  //     de cap arbitrário/vsync-adaptativo aqui, semear um desdobramento se um consumidor pedir)
  //     -- rejeitar aqui significa que um chamador nunca entra silenciosamente num comportamento
  //     de driver que esta API não foi desenhada pra prometer. CONTEXTO (D-ORDEM):
  //     `glfwSwapInterval` exige o contexto GL CORRENTE na thread chamadora; o create() acima já
  //     torna o contexto de `win_` corrente logo após `glfwCreateWindow` e esta classe nunca o
  //     troca pelo resto da vida do App (o mesmo invariante de contexto único que as chamadas
  //     diretas `glReadPixels`/`glBindFramebuffer` de `render()`/`snapshot()` em app.cpp já
  //     assumem sem reafirmá-lo) -- então esta chamada NÃO rechama `glfwMakeContextCurrent`
  //     sozinha; só é seguro chamar depois de um create() bem-sucedido, o que a guarda de `win_`
  //     nulo acima já garante. Idempotente: chamar com o MESMO intervalo duas vezes (ou qualquer
  //     intervalo) é um pedido seguro e barato de no-op-se-inalterado ao driver -- GLFW/o driver
  //     são donos de qualquer estado de fato, esta classe não guarda nenhum próprio (diferente
  //     de `dispatched_mode_` pro modo de janela) porque não existe query do GLFW pra desincronizar
  //     dele. HONESTIDADE DE DRIVER: igual ao `set_mode()` acima, isto é um PEDIDO -- alguns
  //     drivers/compositores fixam ou ignoram (limitação documentada do GLFW, não da glintfx);
  //     `get_monitor_refresh_hz()` abaixo é um caminho de LEITURA SEPARADO, nunca um eco do
  //     intervalo pedido aqui.
  bool set_swap_interval(int interval);

  // EN: WM-VSYNC -- query the refresh rate (Hz) of the monitor the window is CURRENTLY on, so a
  //     host can reason about half-rate/third-rate `set_swap_interval` targets or a HUD readout
  //     without depending on `set_swap_interval`'s own request having been honoured. Returns `0`
  //     when `win_` is null, no monitor can be resolved, or the resolved monitor reports no
  //     video mode (D5 fail-high, twin of create()'s own null-vm guard) -- documented
  //     INDETERMINATE, not an error: headless (Xvfb, no real display) is the expected case that
  //     hits this path, not a bug. MONITOR RESOLUTION (cheap: `glfwGetMonitors()` is O(few) on
  //     every real desktop, this is not a per-frame call): fullscreen
  //     (`glfwGetWindowMonitor(win_) != nullptr`, same signal `mode()` above reads) uses that
  //     EXACT monitor -- GLFW already knows it, no guessing needed. Otherwise (Windowed/
  //     Maximized) GLFW has NO "which monitor is this window on" query of its own, so
  //     `monitor_for_window()` (private, below) computes the monitor with the LARGEST rectangle
  //     overlap against the window's current position+size across `glfwGetMonitors()` -- correct
  //     multi-monitor behaviour (distribution-general scope, not just the primary), falling back
  //     to the primary monitor only when NO monitor overlaps at all (observed under Xvfb: the
  //     virtual display's reported geometry does not always overlap the window's reported
  //     position, since there is no real WM placing it) -- see that method's own doc-comment for
  //     the overlap arithmetic.
  // PT: WM-VSYNC -- consulta a taxa de atualização (Hz) do monitor em que a janela está
  //     ATUALMENTE, para que um host possa raciocinar sobre alvos de `set_swap_interval` de
  //     meia-taxa/terça-taxa ou uma leitura de HUD sem depender do próprio pedido de
  //     `set_swap_interval` ter sido honrado. Retorna `0` quando `win_` é nulo, nenhum monitor
  //     é resolvível, ou o monitor resolvido não reporta video mode (fail-high D5, gêmeo da
  //     própria guarda de vm nulo do create()) -- documentado como INDETERMINADO, não um erro:
  //     headless (Xvfb, sem display real) é o caso esperado que bate neste caminho, não um bug.
  //     RESOLUÇÃO DE MONITOR (barato: `glfwGetMonitors()` é O(poucos) em todo desktop real, isto
  //     não é uma chamada por-frame): fullscreen (`glfwGetWindowMonitor(win_) != nullptr`, o
  //     mesmo sinal que mode() acima lê) usa EXATAMENTE aquele monitor -- o GLFW já sabe, sem
  //     precisar adivinhar. Senão (Windowed/Maximized) o GLFW NÃO tem query própria de "em qual
  //     monitor esta janela está", então `monitor_for_window()` (privado, abaixo) computa o
  //     monitor com o MAIOR overlap de retângulo contra a posição+tamanho corrente da janela
  //     através de `glfwGetMonitors()` -- comportamento multi-monitor correto (escopo de
  //     distribuição geral, não só o primário), caindo pro monitor primário só quando NENHUM
  //     monitor tem overlap nenhum (observado sob Xvfb: a geometria reportada do display
  //     virtual nem sempre sobrepõe a posição reportada da janela, já que não há WM real
  //     posicionando-a) -- ver o doc-comment daquele método pra aritmética do overlap.
  int get_monitor_refresh_hz() const;

  // EN: Register the sink that receives physical input translated to the neutral UiEvent
  //     format (A1, framework-2D). The 5 GLFW callbacks (key/char/mouse-button/cursor-pos/
  //     scroll) are always registered inside create() -- independent of whether a sink has
  //     been set yet -- and read the CURRENT sink_ via the stored WindowGlfw* user pointer
  //     every time they fire (glfwSetWindowUserPointer), not a snapshot taken at registration
  //     time; so calling set_event_sink() at any point after create() (including from inside
  //     App's constructor, after engine.attach() succeeds -- see app.cpp) is enough to start
  //     receiving events on the next poll(). A null/empty sink is a safe no-op: events are
  //     silently discarded, same "nothing happens" contract poll() already had before A1.
  // PT: Registra o sink que recebe input físico traduzido para o formato neutro UiEvent (A1,
  //     framework-2D). Os 5 callbacks GLFW (key/char/botão-do-mouse/cursor/scroll) são sempre
  //     registrados dentro de create() -- independente de um sink já ter sido definido -- e
  //     leem o sink_ CORRENTE via o ponteiro de usuário WindowGlfw* armazenado
  //     (glfwSetWindowUserPointer) toda vez que disparam, não um snapshot tirado no momento do
  //     registro; então chamar set_event_sink() a qualquer momento após create() (inclusive de
  //     dentro do construtor do App, após engine.attach() ter sucesso -- ver app.cpp) já basta
  //     para começar a receber eventos no próximo poll(). Um sink nulo/vazio é um no-op
  //     seguro: eventos são descartados silenciosamente, mesmo contrato "nada acontece" que
  //     poll() já tinha antes do A1.
  void set_event_sink(std::function<void(const UiEvent&)> sink);

  // EN: HOSTIN-1 (Onda 2) -- physical input STATE queries, fed by the SAME 5 GLFW callbacks
  //     above (D2: the state is updated FIRST, inside handle_key/handle_mouse_button/
  //     handle_cursor_pos, then the UiEvent sink is dispatched, so a sink callback that reads
  //     one of these mid-frame sees fresh state). Fail-high: false/(0,0) before the window ever
  //     receives a matching physical event, or for an out-of-range Key/button (see
  //     input_state.hpp for the exact guard). `App::process_event()` (the SYNTHETIC channel)
  //     never writes here (D5) -- see this file's own header comment.
  // PT: HOSTIN-1 (Onda 2) -- consultas de ESTADO de input físico, alimentadas pelos MESMOS 5
  //     callbacks GLFW acima (D2: o estado é atualizado PRIMEIRO, dentro de handle_key/
  //     handle_mouse_button/handle_cursor_pos, e só então o sink de UiEvent é despachado, então
  //     um callback do sink que lê uma destas no meio do frame vê estado já fresco). Fail-high:
  //     false/(0,0) antes da janela receber algum evento físico correspondente, ou para um
  //     Key/botão fora de faixa (ver input_state.hpp pro guard exato). `App::process_event()`
  //     (o canal SINTÉTICO) nunca escreve aqui (D5) -- ver o próprio comentário de cabeçalho
  //     deste arquivo.
  bool is_key_down(Key k) const { return input_state_.is_key_down(k); }
  bool is_mouse_button_down(int button) const { return input_state_.is_mouse_button_down(button); }
  void get_cursor_pos(float& x, float& y) const { input_state_.get_cursor_pos(x, y); }

  // EN: HOSTIN-2 (Onda 2) -- edge-detected key callback, registerable/replaceable at any point;
  //     null/empty is a safe no-op (same convention as set_event_sink above). Fires from inside
  //     handle_key(), which runs synchronously from glfwPollEvents() (i.e. inside App::
  //     poll_events()) -- copy-before-invoke discipline (AUD-TEC-3) applies, same as every other
  //     callback in this library: a callback that re-registers a new key callback from within
  //     its own invocation cannot destroy the std::function object currently running it.
  // PT: HOSTIN-2 (Onda 2) -- callback de tecla edge-detectado, registrável/substituível a
  //     qualquer momento; nulo/vazio é um no-op seguro (mesma convenção do set_event_sink
  //     acima). Dispara de dentro de handle_key(), que roda sincronamente a partir de
  //     glfwPollEvents() (ou seja, dentro de App::poll_events()) -- disciplina de
  //     cópia-antes-de-invocar (AUD-TEC-3) se aplica, igual a todo outro callback desta
  //     biblioteca: um callback que reregistra um novo callback de tecla de dentro da própria
  //     invocação não consegue destruir o objeto std::function que o está rodando agora.
  void set_key_callback(std::function<void(Key key, KeyAction action, int modifiers)> cb);

  // EN: HOSTIN-3 (Onda 2, D6) -- programmatic quit. BYPASSES the close-request veto below
  //     entirely: this is glfwSetWindowShouldClose(win_, GLFW_TRUE) directly, which does NOT
  //     invoke the GLFW close callback (that callback only fires for a USER-initiated close
  //     attempt -- X button, Alt+F4). No-op when win_ is null.
  // PT: HOSTIN-3 (Onda 2, D6) -- saída programática. BYPASSA o veto de pedido-de-close abaixo
  //     inteiramente: isto é glfwSetWindowShouldClose(win_, GLFW_TRUE) direto, que NÃO invoca o
  //     callback de close do GLFW (aquele callback só dispara pra uma tentativa de close
  //     INICIADA PELO USUÁRIO -- botão X, Alt+F4). No-op quando win_ é nulo.
  void request_close();

  // EN: HOSTIN-4 (Onda 2, D6) -- registers the USER-initiated close veto: `cb()` is invoked
  //     from inside handle_window_close() (fired by the GLFW close callback, itself triggered
  //     only by the window system -- X button, Alt+F4 -- never by request_close() above);
  //     `false` resets glfwWindowShouldClose back to GLFW_FALSE (window stays open, veto), any
  //     other outcome (`true`, or no callback registered at all) lets the close proceed. See
  //     glfw_decide_window_close() (glfw_event_translate.hpp) for the extracted pure decision
  //     this wraps, and this file's own header comment for the copy-before-invoke discipline.
  // PT: HOSTIN-4 (Onda 2, D6) -- registra o veto de close INICIADO PELO USUÁRIO: `cb()` é
  //     invocado de dentro de handle_window_close() (disparado pelo callback de close do GLFW,
  //     ele mesmo disparado só pelo sistema de janelas -- botão X, Alt+F4 -- nunca pelo
  //     request_close() acima); `false` reseta glfwWindowShouldClose de volta a GLFW_FALSE
  //     (janela fica aberta, veto), qualquer outro resultado (`true`, ou nenhum callback
  //     registrado) deixa o close prosseguir. Ver glfw_decide_window_close()
  //     (glfw_event_translate.hpp) pra decisão pura extraída que isto encapsula, e o próprio
  //     comentário de cabeçalho deste arquivo pra disciplina de cópia-antes-de-invocar.
  void set_close_request_callback(std::function<bool()> cb);

  // EN: HOSTIN-5 (Onda 2, D7) -- WINDOW focus/iconify observation. Prefixed `window_` so these
  //     never collide with the pre-existing DOM element focus concept (App::set_focus_callback,
  //     app.hpp -- a completely different thing: keyboard-input-focused RmlUi element vs. this
  //     OS-level window having input focus at all). Fail-high: false when !win_. Headless
  //     honesty (documented, same class as WindowMode::mode()'s own Xvfb note): under Xvfb
  //     there is no window manager, so iconify may never be honoured and focus reporting can be
  //     degenerate -- the callbacks/queries are exercised for real only in the nested-compositor
  //     manual QA leg (see docs/superpowers/plans/2026-07-22-onda2-input-host.md section 4.3).
  // PT: HOSTIN-5 (Onda 2, D7) -- observação de focus/iconify de JANELA. Prefixado `window_` pra
  //     nunca colidir com o conceito pré-existente de foco de elemento DOM
  //     (App::set_focus_callback, app.hpp -- uma coisa completamente diferente: elemento RmlUi
  //     com foco de input de teclado vs. esta janela no nível do SO ter foco de input). Fail-
  //     high: false quando !win_. Honestidade headless (documentada, mesma classe da própria
  //     nota de Xvfb de WindowMode::mode()): sob Xvfb não há window manager, então iconify pode
  //     nunca ser honrado e o relato de focus pode ser degenerado -- os callbacks/queries são
  //     exercitados de verdade só na perna manual de QA em compositor aninhado (ver
  //     docs/superpowers/plans/2026-07-22-onda2-input-host.md seção 4.3).
  void set_window_focus_callback(std::function<void(bool focused)> cb);
  void set_window_iconify_callback(std::function<void(bool iconified)> cb);
  bool is_window_focused() const;
  bool is_window_iconified() const;

  // EN: WIN-ICON (framework-2D) -- sets the window/taskbar icon from a caller-owned RGBA8 pixel
  //     buffer already in memory (the consumer decodes its own PNG, e.g. with stb_image, and
  //     hands the decoded pixels here -- this call never touches a file path or does any
  //     decoding of its own). Wraps `glfwSetWindowIcon()` directly. VOCABULARY, deliberately the
  //     SAME as `glintfx::Draw2d::PixelFormat::Rgba8` (`glintfx/include/glintfx/draw2d.hpp`,
  //     D2D-TEXPIXELS) for the pixel LAYOUT -- 8 bits per channel, red first, row-major,
  //     top-left origin, `w * h * 4` bytes total -- but this call does NOT reuse that enum type
  //     (window chrome has no dependency on the Draw2D module, which is independently
  //     OFF-able) and, UNLIKE `Draw2d::create_texture`'s own `Rgba8`, does NOT premultiply
  //     alpha on ingest: `Draw2d`'s premultiply exists because ITS pixels feed glintfx's own
  //     `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` blend pipeline (`docs/draw2d.md`'s "Premultiply and the
  //     tint formula" section); an OS window/taskbar icon is composited by the WINDOW MANAGER,
  //     never by glintfx's own GL blend state, so `pixels_rgba8` is taken STRAIGHT
  //     (non-premultiplied) -- the same convention GLFW's own `GLFWimage` documentation states
  //     for `glfwSetWindowIcon`. `pixels_rgba8` is read SYNCHRONOUSLY and copied into an
  //     internal buffer before this call returns (GLFW itself converts to the platform's native
  //     icon format synchronously inside `glfwSetWindowIcon`, keeping no reference afterwards)
  //     -- safe to free/reuse the caller's own buffer the instant this call returns.
  //
  //     Fail-high (D5's own discipline, same shape as `create()`'s non-positive-size guard
  //     above), guard order literal, every check BEFORE touching `pixels_rgba8`: `win_ == nullptr`
  //     (`create()` not called or failed) -> `false`; `pixels_rgba8 == nullptr` -> `false`;
  //     `w <= 0 || h <= 0` -> `false`; `w > kMaxIconDim || h > kMaxIconDim` (2048, generous
  //     headroom over any real OS/WM icon size while bounding the worst-case copy to 16 MiB and
  //     keeping `w * h * 4` far below any `int`/`size_t` overflow BEFORE the multiply ever runs,
  //     window_glfw.cpp's own doc-comment on this method has the full derivation) -> `false` --
  //     one dedup'd stderr line per rejection, never a crash, never a read past the caller's
  //     buffer. `nullptr` is REJECTED, not treated as "reset to the default icon" (GLFW's own
  //     `count == 0` convention for that) -- out of scope for this slice by design, a follow-up
  //     if a consumer asks for it explicitly.
  //
  //     No return-value distinction between "rejected by this guard" and "GLFW/the WM silently
  //     ignored the request" -- `glfwSetWindowIcon` itself returns `void` and some platforms
  //     (documented GLFW limitation, e.g. certain window managers with no icon concept at all)
  //     simply do nothing with a well-formed request; this call cannot observe that and does not
  //     claim to.
  // PT: WIN-ICON (framework-2D) -- define o ícone de janela/barra de tarefas a partir de um
  //     buffer de pixel RGBA8 já em memória, de posse do chamador (o consumidor decodifica o
  //     próprio PNG, ex.: com stb_image, e entrega os pixels decodificados aqui -- esta chamada
  //     nunca toca um caminho de arquivo nem faz decode nenhum por conta própria). Encapsula
  //     `glfwSetWindowIcon()` diretamente. VOCABULÁRIO, deliberadamente o MESMO de
  //     `glintfx::Draw2d::PixelFormat::Rgba8` (`glintfx/include/glintfx/draw2d.hpp`,
  //     D2D-TEXPIXELS) pro LAYOUT de pixel -- 8 bits por canal, vermelho primeiro, row-major,
  //     origem superior-esquerda, `w * h * 4` bytes no total -- mas esta chamada NÃO reusa
  //     aquele tipo enum (janela/chrome não tem dependência do módulo Draw2D, que é
  //     independentemente desligável) e, DIFERENTE do próprio `Rgba8` do
  //     `Draw2d::create_texture`, NÃO premultiplica alpha na entrada: o premultiply do `Draw2d`
  //     existe porque os PRÓPRIOS pixels dele alimentam o pipeline de blend
  //     `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` da própria glintfx (seção "Premultiply and the tint
  //     formula" de `docs/draw2d.md`); um ícone de janela/barra de tarefas do SO é composto pelo
  //     WINDOW MANAGER, nunca pelo próprio estado de blend GL da glintfx, então
  //     `pixels_rgba8` é recebido STRAIGHT (não-premultiplicado) -- a mesma convenção que a
  //     própria documentação do `GLFWimage` do GLFW declara para `glfwSetWindowIcon`.
  //     `pixels_rgba8` é lido DE FORMA SÍNCRONA e copiado pra um buffer interno antes desta
  //     chamada retornar (o próprio GLFW converte pro formato de ícone nativo da plataforma de
  //     forma síncrona dentro de `glfwSetWindowIcon`, sem reter referência depois) -- seguro
  //     liberar/reusar o buffer do próprio chamador no instante em que esta chamada retorna.
  //
  //     Fail-high (a própria disciplina do D5, mesma forma da guarda de tamanho não-positivo do
  //     `create()` acima), ordem de guarda literal, toda checagem ANTES de tocar
  //     `pixels_rgba8`: `win_ == nullptr` (`create()` não chamado ou falhou) -> `false`;
  //     `pixels_rgba8 == nullptr` -> `false`; `w <= 0 || h <= 0` -> `false`;
  //     `w > kMaxIconDim || h > kMaxIconDim` (2048, folga generosa sobre qualquer tamanho real
  //     de ícone de SO/WM enquanto limita a cópia de pior caso a 16 MiB e mantém `w * h * 4`
  //     bem abaixo de qualquer overflow de `int`/`size_t` ANTES da multiplicação sequer rodar,
  //     o próprio doc-comment deste método em window_glfw.cpp tem a derivação completa) ->
  //     `false` -- uma linha de stderr dedup'd por classe de rejeição, nunca um crash, nunca
  //     uma leitura além do buffer do chamador. `nullptr` é REJEITADO, não tratado como
  //     "restaurar o ícone default" (a própria convenção `count == 0` do GLFW pra isso) -- fora
  //     de escopo desta fatia por desenho, um desdobramento se um consumidor pedir
  //     explicitamente.
  //
  //     Sem distinção de valor de retorno entre "rejeitado por esta guarda" e "GLFW/o WM
  //     ignorou o pedido silenciosamente" -- o próprio `glfwSetWindowIcon` retorna `void` e
  //     algumas plataformas (limitação documentada do GLFW, ex.: certos window managers sem
  //     conceito de ícone nenhum) simplesmente não fazem nada com um pedido bem-formado; esta
  //     chamada não consegue observar isso e não alega o contrário.
  bool set_window_icon(const void* pixels_rgba8, int w, int h);

private:
  GLFWwindow* win_        = nullptr;
  bool        glfw_inited_ = false; // EN: true only after glfwInit() succeeds. PT: true somente após glfwInit() com sucesso.
  std::function<void(const UiEvent&)> sink_;

  // EN: A4-WINMODES -- mode-transition state. cfg_w_/cfg_h_ are the AppConfig construction
  //     dimensions (create()'s w/h args), used both for a FullscreenExclusive create()/set_mode()
  //     target (GLFW picks the closest matching video mode) and as the D7 restore-to-windowed
  //     fallback size for a window that was NEVER windowed (born Maximized/Fullscreen).
  //     fullscreen_desktop_ disambiguates Desktop from Exclusive while glfwGetWindowMonitor()
  //     reports "some fullscreen" for both (D4 -- GLFW itself does not track the distinction).
  //     saved_x_/saved_y_/saved_w_/saved_h_ + has_saved_geometry_ are the D7 windowed-geometry
  //     snapshot, captured by save_windowed_geometry() the moment mode() reads Windowed just
  //     before set_mode() dispatches to a different target; saved_x_/saved_y_ default to
  //     (64,64) (D7's documented "never was windowed" position) so a fullscreen/maximized-born
  //     window that later asks for Windowed never materialises at (0,0), under a panel.
  // PT: A4-WINMODES -- estado de transição de modo. cfg_w_/cfg_h_ são as dimensões de
  //     construção do AppConfig (args w/h do create()), usadas tanto como alvo de
  //     create()/set_mode() FullscreenExclusive (o GLFW escolhe o video mode mais próximo)
  //     quanto como tamanho de fallback D7 pra restaurar-a-windowed de uma janela que NUNCA
  //     foi windowed (nasceu Maximized/Fullscreen). fullscreen_desktop_ desambigua Desktop de
  //     Exclusive enquanto glfwGetWindowMonitor() reporta "algum fullscreen" pros dois (D4 -- o
  //     próprio GLFW não rastreia a distinção). saved_x_/saved_y_/saved_w_/saved_h_ +
  //     has_saved_geometry_ são o snapshot de geometria windowed do D7, capturado por
  //     save_windowed_geometry() no instante em que mode() lê Windowed logo antes de
  //     set_mode() despachar pra um alvo diferente; saved_x_/saved_y_ têm default (64,64) (a
  //     posição documentada "nunca foi windowed" do D7) para que uma janela nascida
  //     fullscreen/maximized que depois peça Windowed nunca materialize em (0,0), sob um painel.
  int  cfg_w_ = 0;
  int  cfg_h_ = 0;
  bool fullscreen_desktop_   = false;
  bool has_saved_geometry_   = false;
  int  saved_x_ = 64;
  int  saved_y_ = 64;
  int  saved_w_ = 0;
  int  saved_h_ = 0;

  // EN: A4-WINMODES review fix (MAJOR #1, 2026-07-21) -- the mode WE last DISPATCHED to the
  //     window system (set synchronously by create() and by every dispatching branch of
  //     set_mode()), used for set_mode()'s no-op/branch decisions INSTEAD OF the live mode()
  //     query. glfwGetWindowAttrib(win_, GLFW_MAXIMIZED) -- the signal mode() reads for
  //     Maximized -- is answered by the WM/compositor ASYNCHRONOUSLY on real desktops
  //     (confirmed on live KDE/Wayland: it can take more than one poll+render cycle to reflect
  //     a glfwMaximizeWindow()/glfwRestoreWindow() call). Reasoning about "what should this
  //     call do" from that laggy read let a Windowed request immediately following a Maximized
  //     request read the STALE pre-maximize attrib, mistake it for "already Windowed", and
  //     silently no-op -- stranding the window maximized forever once the WM's confirmation
  //     eventually landed (repro 3/3 on a real session, adversarial review finding).
  //     dispatched_mode_ is OUR OWN synchronous ledger of intent and never lags, so using it for
  //     the transition logic makes every set_mode() call deterministic regardless of how fast
  //     the WM/compositor gets around to confirming the previous one. window_mode() (the PUBLIC
  //     query, mode() below) is UNCHANGED and still derives live from the window system -- it
  //     honestly reports the async lag as a documented characteristic (D4) rather than hiding
  //     it; only this INTERNAL bookkeeping stops trusting the laggy read for its OWN decisions.
  // PT: Fix do review A4-WINMODES (MAJOR #1, 2026-07-21) -- o modo que NÓS últimos despachamos
  //     ao sistema de janelas (setado sincronamente pelo create() e por todo ramo que despacha
  //     do set_mode()), usado nas decisões de no-op/ramo do set_mode() EM VEZ DA query viva
  //     mode(). glfwGetWindowAttrib(win_, GLFW_MAXIMIZED) -- o sinal que mode() lê pra
  //     Maximized -- é respondido pelo WM/compositor de forma ASSÍNCRONA em desktops reais
  //     (confirmado no KDE/Wayland ao vivo: pode levar mais de um ciclo poll+render pra refletir
  //     uma chamada glfwMaximizeWindow()/glfwRestoreWindow()). Raciocinar sobre "o que esta
  //     chamada deve fazer" a partir dessa leitura atrasada deixava um pedido Windowed logo após
  //     um pedido Maximized ler o attrib PRÉ-maximizar desatualizado, confundi-lo com "já
  //     Windowed", e virar no-op silencioso -- travando a janela maximizada pra sempre assim que
  //     a confirmação do WM finalmente chegasse (repro 3/3 numa sessão real, achado do review
  //     adversarial). dispatched_mode_ é nosso PRÓPRIO livro-razão síncrono de intenção e nunca
  //     atrasa, então usá-lo na lógica de transição torna toda chamada set_mode() determinística
  //     independente de quão rápido o WM/compositor confirma a anterior. window_mode() (a query
  //     PÚBLICA, mode() abaixo) fica INALTERADA e continua derivando ao vivo do sistema de
  //     janelas -- reporta honestamente o atraso assíncrono como uma característica documentada
  //     (D4) em vez de escondê-la; só esta contabilidade INTERNA para de confiar na leitura
  //     atrasada pras próprias decisões dela.
  WindowMode dispatched_mode_ = WindowMode::Windowed;

  // EN: A4-WINMODES review follow-up (found during real-session verification, 2026-07-21,
  //     beyond the review's original MAJOR #1/#2/(d) list) -- every glfwSetWindowMonitor()/
  //     glfwMaximizeWindow()/glfwRestoreWindow() call in set_mode() is a REQUEST to the Wayland
  //     compositor, not a synchronous state change. Calling set_mode() again immediately
  //     afterwards (either from within the SAME call, the Fullscreen->Maximized D8 sequence, or
  //     from the CALLER issuing a second set_mode() one frame later) can race the compositor's
  //     confirmation: it may still be applying the FIRST request when the second lands, and
  //     capture the wrong "restore-to" geometry or settle on the wrong final state (reproduced
  //     live: a resized-then-fullscreen-then-restored window landed at monitor resolution
  //     instead of the tracked windowed size). Called at the end of every window-system-
  //     touching branch of set_mode(), right before it returns -- a BOUNDED wait
  //     (glfwWaitEventsTimeout(), GLFW's own blocking-wait-with-deadline primitive, NOT a blind
  //     sleep) that gives the compositor's configure/ack round-trip a window to land before the
  //     call returns control to the caller, so the NEXT set_mode() call (whenever it comes)
  //     starts from settled ground truth.
  // PT: Desdobramento do review A4-WINMODES (achado durante a verificação em sessão real,
  //     2026-07-21, além da lista original MAJOR #1/#2/(d)) -- toda chamada
  //     glfwSetWindowMonitor()/glfwMaximizeWindow()/glfwRestoreWindow() em set_mode() é um
  //     PEDIDO ao compositor Wayland, não uma troca de estado síncrona. Chamar set_mode() de
  //     novo logo em seguida (seja de DENTRO da MESMA chamada, na sequência D8 Fullscreen->
  //     Maximized, seja do CHAMADOR emitindo um segundo set_mode() um frame depois) pode entrar
  //     em race com a confirmação do compositor: ele pode ainda estar aplicando o PRIMEIRO
  //     pedido quando o segundo chega, e capturar a geometria "restaurar-para" errada ou
  //     assentar no estado final errado (reproduzido ao vivo: uma janela redimensionada-depois-
  //     fullscreen-depois-restaurada pousava na resolução do monitor em vez do tamanho windowed
  //     rastreado). Chamado no fim de todo ramo de set_mode() que toca o sistema de janelas,
  //     bem antes de retornar -- uma espera LIMITADA (glfwWaitEventsTimeout(), o próprio
  //     primitivo bloqueante-com-prazo do GLFW, NÃO um sleep cego) que dá à ida-e-volta de
  //     configure/ack do compositor uma janela pra aterrissar antes da chamada devolver o
  //     controle ao chamador, pra que a PRÓXIMA chamada set_mode() (quando quer que venha)
  //     comece de um chão de verdade assentado.
  static void settle_window_system_request();

  void save_windowed_geometry();

  // EN: WM-VSYNC -- resolves the monitor with the LARGEST overlap against the window's current
  //     `glfwGetWindowPos`/`glfwGetWindowSize` rectangle, iterating `glfwGetMonitors()` (a
  //     handful of entries on every real desktop -- cheap, not a per-frame call). Falls back to
  //     `glfwGetPrimaryMonitor()` when no monitor overlaps at all (`win_` null, no monitors
  //     reported, or every candidate's overlap area is zero -- observed under Xvfb). Only called
  //     by `get_monitor_refresh_hz()` (its public doc-comment, above, carries the full contract
  //     this backs) for the non-fullscreen case -- fullscreen already has the exact monitor from
  //     `glfwGetWindowMonitor(win_)`, no need to guess.
  // PT: WM-VSYNC -- resolve o monitor com o MAIOR overlap contra o retângulo corrente da janela
  //     (`glfwGetWindowPos`/`glfwGetWindowSize`), iterando `glfwGetMonitors()` (um punhado de
  //     entradas em todo desktop real -- barato, não é chamada por-frame). Cai pra
  //     `glfwGetPrimaryMonitor()` quando nenhum monitor tem overlap nenhum (`win_` nulo, nenhum
  //     monitor reportado, ou todo candidato com área de overlap zero -- observado sob Xvfb). Só
  //     chamado por `get_monitor_refresh_hz()` (seu doc-comment público, acima, carrega o
  //     contrato completo que isto sustenta) pro caso não-fullscreen -- fullscreen já tem o
  //     monitor exato via `glfwGetWindowMonitor(win_)`, sem precisar adivinhar.
  //
  // EN: COVERAGE GAP (found by adversarial review, w18-rev2) -- the overlap ARITHMETIC above
  //     (largest-rectangle-overlap selection across multiple candidate monitors) has NO
  //     automated test proving it picks the CORRECT monitor. Measured: sabotaging this method to
  //     unconditionally `return glfwGetPrimaryMonitor();` (skipping the overlap loop entirely)
  //     survives the FULL suite (102/102 GLFW=ON) -- because Xvfb always reports exactly ONE
  //     virtual monitor, "largest overlap" and "always the primary" are INDISTINGUISHABLE in
  //     every environment this suite runs in. The suite only proves this method does not crash
  //     and returns a monitor whose `glfwGetVideoMode()` is readable -- NOT that the overlap
  //     comparison itself is correct when more than one monitor is a candidate. Same class of
  //     limitation as the window-focus/iconify observation: only a REAL multi-monitor desktop
  //     session (nested compositor or otherwise) would exercise more than one candidate and
  //     prove the arithmetic picks the right one. Whoever touches this method's overlap logic
  //     next: the suite will NOT catch a wrong answer here, only a crash.
  // PT: LACUNA DE COBERTURA (achada por review adversarial, w18-rev2) -- a ARITMÉTICA de overlap
  //     acima (seleção por maior overlap de retângulo entre múltiplos monitores candidatos) NÃO
  //     tem teste automatizado provando que escolhe o monitor CORRETO. Medido: sabotar este
  //     método pra `return glfwGetPrimaryMonitor();` incondicional (pulando o laço de overlap
  //     inteiro) sobrevive à suíte COMPLETA (102/102 GLFW=ON) -- porque o Xvfb sempre reporta
  //     exatamente UM monitor virtual, "maior overlap" e "sempre o primário" são
  //     INDISTINGUÍVEIS em todo ambiente onde esta suíte roda. A suíte só prova que este método
  //     não crasha e retorna um monitor cujo `glfwGetVideoMode()` é legível -- NÃO que a própria
  //     comparação de overlap está correta quando mais de um monitor é candidato. Mesma classe
  //     de limitação da observação de focus/iconify de janela: só uma sessão de desktop
  //     multi-monitor REAL (compositor aninhado ou não) exercitaria mais de um candidato e
  //     provaria que a aritmética escolhe o certo. Quem mexer na lógica de overlap deste método
  //     depois: a suíte NÃO vai pegar uma resposta errada aqui, só um crash.
  GLFWmonitor* monitor_for_window() const;

  // EN: Last known modifier bitmask (glintfx::Mod), updated by the key and mouse-button
  //     trampolins and reused for the cursor-pos/scroll callbacks, which GLFW does NOT hand a
  //     mods parameter to -- same pattern the pinned RmlUi GLFW backend uses
  //     (glfw_active_modifiers, RmlUi_Backend_GLFW_GL3.cpp).
  // PT: Último bitmask de modificador conhecido (glintfx::Mod), atualizado pelos trampolins de
  //     tecla e botão-do-mouse e reusado pelos callbacks de cursor/scroll, aos quais o GLFW
  //     NÃO entrega um parâmetro mods -- mesmo padrão que o backend GLFW pinado do RmlUi usa
  //     (glfw_active_modifiers, RmlUi_Backend_GLFW_GL3.cpp).
  int active_mods_ = 0;

  // EN: HOSTIN-1 (Onda 2) -- the physical input state table, updated by handle_key/
  //     handle_mouse_button/handle_cursor_pos BEFORE they dispatch to sink_ (D2). Never touched
  //     by App::process_event() (the synthetic channel, D5).
  // PT: HOSTIN-1 (Onda 2) -- a tabela de estado de input físico, atualizada por handle_key/
  //     handle_mouse_button/handle_cursor_pos ANTES de despacharem para sink_ (D2). Nunca
  //     tocada por App::process_event() (o canal sintético, D5).
  InputState input_state_;

  // EN: HOSTIN-2/4/5 (Onda 2) -- optional host callbacks. All null/empty by default (safe
  //     no-op, same convention as sink_ above); all invoked via a LOCAL COPY first (AUD-TEC-3
  //     reentrancy discipline, same as every other callback family in this library).
  // PT: HOSTIN-2/4/5 (Onda 2) -- callbacks opcionais do host. Todos nulos/vazios por padrão
  //     (no-op seguro, mesma convenção do sink_ acima); todos invocados via uma CÓPIA LOCAL
  //     primeiro (disciplina de reentrância AUD-TEC-3, igual a toda outra família de callback
  //     desta biblioteca).
  std::function<void(Key, KeyAction, int)> key_callback_;
  std::function<bool()> close_request_callback_;
  std::function<void(bool)> window_focus_callback_;
  std::function<void(bool)> window_iconify_callback_;

  // EN: One-line static trampolines registered with glfwSet*Callback -- resolve the
  //     WindowGlfw* from glfwGetWindowUserPointer and call into the corresponding instance
  //     method below, which does the actual translation (via glfw_event_translate.hpp) and
  //     sink dispatch. Kept to a single line each per the plan's testability contract: all
  //     real logic lives in the pure functions the unit test exercises directly.
  // PT: Trampolins static de uma linha registrados com glfwSet*Callback -- resolvem o
  //     WindowGlfw* via glfwGetWindowUserPointer e chamam o método de instância
  //     correspondente abaixo, que faz a tradução de fato (via glfw_event_translate.hpp) e o
  //     despacho ao sink. Mantidos a uma linha cada, conforme o contrato de testabilidade do
  //     plano: toda a lógica real mora nas funções puras que o teste unit exercita direto.
  static void on_key(GLFWwindow* w, int key, int scancode, int action, int mods);
  static void on_char(GLFWwindow* w, unsigned int codepoint);
  static void on_mouse_button(GLFWwindow* w, int button, int action, int mods);
  static void on_cursor_pos(GLFWwindow* w, double xpos, double ypos);
  static void on_scroll(GLFWwindow* w, double xoffset, double yoffset);
  // EN: HOSTIN-3/4/5 (Onda 2) -- the 3 new trampolines this wave adds, same one-line contract.
  // PT: HOSTIN-3/4/5 (Onda 2) -- os 3 trampolins novos que esta onda adiciona, mesmo contrato
  //     de uma linha.
  static void on_window_close(GLFWwindow* w);
  static void on_window_focus(GLFWwindow* w, int focused);
  static void on_window_iconify(GLFWwindow* w, int iconified);

  void dispatch(const UiEvent& ev);
  void handle_key(int key, int scancode, int action, int mods);
  void handle_char(unsigned int codepoint);
  void handle_mouse_button(int button, int action, int mods);
  void handle_cursor_pos(double xpos, double ypos);
  void handle_scroll(double xoffset, double yoffset);
  void handle_window_close();
  void handle_window_focus(int focused);
  void handle_window_iconify(int iconified);
};
}
