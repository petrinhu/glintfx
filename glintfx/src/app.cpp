// SPDX-License-Identifier: Apache-2.0
// EN: App facade implementation — RAII wrapper over WindowGlfw + SystemInterface_GLFW + Engine.
//     Engine owns RenderGl3 + Bootstrap. SystemInterface_GLFW lives here because it requires a
//     GLFWwindow* at construction time (available only after WindowGlfw::create()). LOGTHR-1
//     (Onda 2, D8): the concrete type constructed below is SystemInterfaceGlfwDedup, a thin
//     subclass adding a dedup/throttle LogMessage override -- see src/system_glfw_dedup.hpp.
// PT: Implementação da fachada App — wrapper RAII sobre WindowGlfw + SystemInterface_GLFW + Engine.
//     Engine possui RenderGl3 + Bootstrap. SystemInterface_GLFW vive aqui pois exige
//     GLFWwindow* na construção (disponível apenas após WindowGlfw::create()). LOGTHR-1
//     (Onda 2, D8): o tipo concreto construído abaixo é SystemInterfaceGlfwDedup, subclasse fina
//     que acrescenta um override de LogMessage com dedup/throttle -- ver src/system_glfw_dedup.hpp.
// Copyright (c) 2026 Petrus Silva Costa
// EN: gl_loader.h for GL function pointers used in snapshot() (glReadPixels, glBindFramebuffer, etc.).
// PT: gl_loader.h para ponteiros de função GL usados em snapshot() (glReadPixels, glBindFramebuffer, etc.).
#include "gl_loader.h"
#include <cstdio>
#include <memory>
#include <string>  // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).
#include <vector>

#include <glintfx/glintfx.hpp>
#include "window_glfw.hpp"
#include "engine.hpp"
// EN: RmlUi_Platform_GLFW.h migrated from bootstrap.cpp — SystemInterface_GLFW now lives here.
// PT: RmlUi_Platform_GLFW.h migrou de bootstrap.cpp — SystemInterface_GLFW agora vive aqui.
#include "RmlUi_Platform_GLFW.h"
// EN: LOGTHR-1 (D8) -- SystemInterfaceGlfwDedup, the dedup/throttle subclass App constructs
//     below instead of the bare upstream SystemInterface_GLFW.
// PT: LOGTHR-1 (D8) -- SystemInterfaceGlfwDedup, a subclasse de dedup/throttle que o App
//     constrói abaixo em vez do SystemInterface_GLFW upstream cru.
#include "system_glfw_dedup.hpp"

namespace glintfx {

// EN: version() lives in src/version.cpp now (L1.9-VERSEMBED) — that TU is always
//     compiled, unlike this one (app.cpp is GLFW-backend-only).
// PT: version() agora vive em src/version.cpp (L1.9-VERSEMBED) — essa TU é sempre
//     compilada, diferente desta (app.cpp é exclusiva do backend GLFW).

// EN: Impl owns all subsystems. Declaration order is intentional: C++ destructs members in
//     REVERSE order, so engine destructs first (calls Rml::Shutdown via Bootstrap::~Bootstrap),
//     then system is deleted (no longer needed by RmlUi), then window dtor destroys GL context.
//     window → system → engine = construction order; engine → system → window = destruction order.
// PT: Impl possui todos os subsistemas. Ordem de declaração é intencional: C++ destrói membros
//     na ORDEM REVERSA, então engine destrói primeiro (chama Rml::Shutdown via Bootstrap::~Bootstrap),
//     depois system é deletado (não mais necessário ao RmlUi), depois dtor de window destrói contexto GL.
//     window → system → engine = ordem de construção; engine → system → window = ordem de destruição.
struct App::Impl {
  WindowGlfw                            window;
  // EN: LOGTHR-1 (D8) -- SystemInterfaceGlfwDedup, not the bare SystemInterface_GLFW (adds the
  //     dedup/throttle LogMessage override; every other method is inherited unchanged).
  // PT: LOGTHR-1 (D8) -- SystemInterfaceGlfwDedup, não o SystemInterface_GLFW cru (acrescenta o
  //     override de LogMessage com dedup/throttle; todo outro método é herdado sem mudança).
  std::unique_ptr<SystemInterfaceGlfwDedup> system; // EN: heap; deleted after engine (RmlUi shutdown).
                                                    // PT: heap; deletado após engine (shutdown RmlUi).
  Engine                                engine;
  int  w  = 0;
  int  h  = 0;
  // EN: Last window framebuffer size seen by render() (AUD-PUB-1 fix). Seeded to the
  //     AppConfig construction size so the first render() frame is a no-op resize (the
  //     Engine::attach() call already set these initial dims -- see App::App() below).
  // PT: Último tamanho de framebuffer da janela visto por render() (fix AUD-PUB-1).
  //     Semeado com o tamanho de construção do AppConfig, então o 1º frame de render() é um
  //     resize no-op (Engine::attach() já setou essas dims iniciais -- ver App::App() abaixo).
  int  last_render_w = 0;
  int  last_render_h = 0;
  bool ok = false;

  // EN: A1 (framework-2D) -- the game's per-frame draw hook + dt bookkeeping. frame_cb is
  //     empty until set_frame_callback() installs one; render() below only measures time and
  //     builds the scene_hook lambda when it is non-empty (zero cost otherwise). frame_cb_seen
  //     tracks whether render() has ever invoked the hook before -- the FIRST invocation
  //     reports dt_seconds=0.0f (no meaningful previous frame to measure against, see
  //     App::set_frame_callback's doc-comment in app.hpp) instead of a huge bogus delta against
  //     last_frame_time's zero-initialised value.
  // PT: A1 (framework-2D) -- o hook de desenho por-frame do jogo + contabilidade de dt.
  //     frame_cb fica vazio até set_frame_callback() instalar um; o render() abaixo só mede
  //     tempo e constrói a lambda scene_hook quando ele não está vazio (custo zero caso
  //     contrário). frame_cb_seen rastreia se o render() já invocou o hook alguma vez antes --
  //     a PRIMEIRA invocação reporta dt_seconds=0.0f (nenhum frame anterior significativo para
  //     medir contra, ver o doc-comment de App::set_frame_callback em app.hpp) em vez de um
  //     delta bogus enorme contra o valor zero-inicializado de last_frame_time.
  std::function<void(float)> frame_cb;
  double                     last_frame_time = 0.0;
  bool                       frame_cb_seen   = false;

  // EN: AUD-PUB-1-TWIN (v0.5.0) -- shared diff-and-set_viewport helper, see the doc-comment on
  //     the out-of-line definition below (just above App::render()) for the full rationale.
  //     Called by BOTH render() and snapshot() so a window resize is picked up identically
  //     regardless of which one runs first after it.
  // PT: AUD-PUB-1-TWIN (v0.5.0) -- helper compartilhado de diff-e-set_viewport, ver o
  //     doc-comment na definição fora-de-linha abaixo (logo acima de App::render()) pra
  //     racional completa. Chamado por AMBOS render() e snapshot() para que um resize de janela
  //     seja capturado identicamente independente de qual dos dois roda primeiro depois dele.
  void sync_viewport(int w, int h);

  // EN: A1 (framework-2D) -- shared render_standalone(+hook) call, same AUD-PUB-1-TWIN
  //     "share it, do not duplicate it" discipline as sync_viewport() above. Without this,
  //     App::snapshot() would silently skip the frame_cb hook the way it used to silently skip
  //     sync_viewport() before the AUD-PUB-1-TWIN fix -- a host driving its game loop through
  //     snapshot() (e.g. an offscreen/CI capture harness) would see a game scene that never
  //     appears in its captured frames, and tests/app_frame_callback_smoke.cpp's pixel-proof
  //     leg (paint inside the hook, then snapshot() to verify composition order) would not be
  //     exercising the hook at all. Measures dt the same way both call sites used to inline it.
  // PT: A1 (framework-2D) -- chamada compartilhada de render_standalone(+hook), mesma
  //     disciplina "compartilhe, não duplique" do AUD-PUB-1-TWIN de sync_viewport() acima. Sem
  //     isto, App::snapshot() puraria silenciosamente o hook frame_cb do mesmo jeito que
  //     pulava silenciosamente sync_viewport() antes do fix AUD-PUB-1-TWIN -- um host que
  //     conduz o loop do jogo através de snapshot() (ex.: um harness de captura offscreen/CI)
  //     veria uma cena de jogo que nunca aparece nos frames capturados, e a perna de prova por
  //     pixel de tests/app_frame_callback_smoke.cpp (pintar dentro do hook, depois snapshot()
  //     para verificar a ordem de composição) não estaria exercitando o hook nenhuma vez. Mede
  //     dt do mesmo jeito que os dois call sites faziam inline antes.
  void render_frame(int w, int h);

  // EN: AUD-PUB-6g-TEARDOWN (bugfix, twin of UiLayer::Impl's own ~Impl(), src/ui_layer.cpp) --
  //     disarms the cursor callback BEFORE the implicit member teardown below runs `engine`'s
  //     destructor (Engine::~Engine -> Bootstrap::~Bootstrap -> Bootstrap::shutdown() ->
  //     Rml::Shutdown()). Same mechanism, same fix, only the SystemInterface type differs
  //     (SystemInterfaceGlfwDedup here vs. SystemClock in the embed half) -- see UiLayer::Impl's
  //     own ~Impl() doc-comment for the full RmlUi-source-confirmed rationale
  //     (Context::~Context() -> UnloadAllDocuments() -> UnloadDocument() -> UpdateHoverChain()
  //     can dispatch one final SetMouseCursor() as the hover chain clears) and
  //     tests/app_cursor_teardown_sanity.cpp for the ASan-provable regression this closes on
  //     the App side (stack-use-after-scope, same shape as cursor_callback_sanity.cpp's own
  //     reentrancy leg (F) that caught the embed-side twin first, CI run 31014883530). `system`
  //     is declared BEFORE `engine` (so it is destroyed AFTER `engine`, per reverse-declaration
  //     order) and therefore remains valid right up through `engine`'s entire destructor --
  //     this call is not a use-after-free of `system`, only a preemptive clear of what it
  //     forwards to. Null-guarded: `system` stays null when App's constructor bails out early
  //     (e.g. WindowGlfw::create() failure) before reaching the
  //     `impl_->system = std::make_unique<...>(...)` line above.
  // PT: AUD-PUB-6g-TEARDOWN (bugfix, gêmeo do próprio ~Impl() de UiLayer::Impl,
  //     src/ui_layer.cpp) -- desarma o callback de cursor ANTES do teardown implícito de membro
  //     abaixo rodar o destrutor do `engine` (Engine::~Engine -> Bootstrap::~Bootstrap ->
  //     Bootstrap::shutdown() -> Rml::Shutdown()). Mesmo mecanismo, mesmo fix, só o tipo de
  //     SystemInterface difere (SystemInterfaceGlfwDedup aqui vs. SystemClock na metade embed)
  //     -- ver o próprio doc-comment do ~Impl() de UiLayer::Impl pro racional completo
  //     confirmado no source do RmlUi (Context::~Context() -> UnloadAllDocuments() ->
  //     UnloadDocument() -> UpdateHoverChain() pode despachar um SetMouseCursor() final
  //     enquanto a hover chain limpa) e tests/app_cursor_teardown_sanity.cpp pra regressão
  //     provável-por-ASan que isto fecha do lado App (stack-use-after-scope, mesma forma da
  //     própria perna de reentrância (F) do cursor_callback_sanity.cpp que pegou o gêmeo
  //     embed-side primeiro, run de CI 31014883530). `system` é declarado ANTES de `engine`
  //     (então é destruído DEPOIS de `engine`, por ordem-reversa-de-declaração) e portanto
  //     continua válido durante todo o destrutor de `engine` -- esta chamada não é um
  //     use-after-free do `system`, só uma limpeza preventiva do que ele repassa. Guarda de
  //     nulo: `system` fica nulo quando o construtor do App sai cedo (ex.: falha do
  //     WindowGlfw::create()) antes de alcançar a linha
  //     `impl_->system = std::make_unique<...>(...)` acima.
  ~Impl() {
    if (system) system->set_cursor_callback(nullptr);
  }
};

App::App(AppConfig cfg) : impl_(std::make_unique<Impl>()) {
  impl_->w = cfg.width;
  impl_->h = cfg.height;
  // EN: A4-WINMODES (D5) -- validate window_mode ONCE, here, before create() ever sees it: an
  //     out-of-range static_cast falls back to Windowed, so the fallback is decided in exactly
  //     one place (not duplicated in WindowGlfw::create() too, even though create() ALSO
  //     defends against an invalid enum reaching it directly -- see its own doc-comment).
  // PT: A4-WINMODES (D5) -- valida window_mode UMA VEZ, aqui, antes do create() sequer vê-lo:
  //     um static_cast fora de faixa cai pra Windowed, então o fallback é decidido em exatamente
  //     um lugar (não duplicado também em WindowGlfw::create(), embora o create() TAMBÉM se
  //     defenda contra um enum inválido chegando direto nele -- ver o próprio doc-comment).
  WindowMode initial_mode = cfg.window_mode;
  switch (initial_mode) {
    case WindowMode::Windowed:
    case WindowMode::Maximized:
    case WindowMode::FullscreenDesktop:
    case WindowMode::FullscreenExclusive:
      break;
    default:
      initial_mode = WindowMode::Windowed;
      break;
  }
  if (!impl_->window.create(cfg.title, cfg.width, cfg.height, initial_mode)) return;
  // EN: Window::create() makes the GL context current — required by Engine::attach().
  // PT: Window::create() torna o contexto GL corrente — exigido por Engine::attach().
  impl_->system = std::make_unique<SystemInterfaceGlfwDedup>(impl_->window.handle());
  // EN: A4-WINMODES (D6) -- attach with the REAL post-create framebuffer size, not the raw
  //     cfg.width/height: a window born Maximized/Fullscreen has a DIFFERENT real framebuffer
  //     at frame 0 (the WM/monitor decides the actual size, not AppConfig), so seeding
  //     Engine::attach()/the AUD-PUB-1 resize cache with cfg dims would render the first frame
  //     with a WRONG layout until sync_viewport() catches up on the next render()/snapshot()
  //     call. Reading window.size() here closes that gap at frame 0 directly, and as a side
  //     effect also fixes a latent HiDPI mismatch for plain Windowed mode (framebuffer size can
  //     differ from the requested window size on a scaled display even without any window-mode
  //     involvement). Windowed at 1x/no HiDPI: real_w/real_h == cfg.width/cfg.height, so this is
  //     a no-op versus the pre-A4-WINMODES behaviour.
  // PT: A4-WINMODES (D6) -- anexa com o tamanho REAL do framebuffer pós-create, não o
  //     cfg.width/height cru: uma janela nascida Maximized/Fullscreen tem um framebuffer REAL
  //     diferente no frame 0 (o WM/monitor decide o tamanho de fato, não o AppConfig), então
  //     semear o Engine::attach()/o cache de resize AUD-PUB-1 com as dims do cfg renderizaria o
  //     1º frame com um layout ERRADO até o sync_viewport() alcançar no próximo render()/
  //     snapshot(). Ler window.size() aqui fecha essa lacuna já no frame 0, e de quebra também
  //     corrige um descasamento latente de HiDPI pro modo Windowed comum (o tamanho do
  //     framebuffer pode diferir do tamanho de janela pedido numa tela escalada mesmo sem
  //     nenhum envolvimento de modo de janela). Windowed em 1x/sem HiDPI: real_w/real_h ==
  //     cfg.width/cfg.height, então isto é um no-op versus o comportamento pré-A4-WINMODES.
  int real_w = 0, real_h = 0;
  impl_->window.size(real_w, real_h);
  if (real_w <= 0 || real_h <= 0) { real_w = cfg.width; real_h = cfg.height; }
  // EN: AUD-APP-MOVEDFROM (W25) -- local reference `impl`, used for the `ok` writes below.
  //     impl_ is guaranteed non-null here (just constructed above), so there is nothing to
  //     null-check in this constructor -- using the reference here keeps the null-safe
  //     dereference pattern this file used to spell out at every entry point reserved for
  //     exactly ONE place: ready()'s own definition, further down (right before App::ok(),
  //     which now just forwards to it).
  // PT: AUD-APP-MOVEDFROM (W25) -- referência local `impl`, usada para as escritas de `ok`
  //     abaixo. impl_ é garantidamente não-nulo aqui (acabou de ser construído acima),
  //     então não há nada para checar contra nulo neste construtor -- usar a referência
  //     aqui mantém o padrão de derreferência null-safe que este arquivo costumava soletrar
  //     em todo ponto de entrada reservado para exatamente UM lugar: a própria definição de
  //     ready(), mais adiante (logo antes de App::ok(), que agora só repassa para ele).
  Impl& impl = *impl_;
  impl.ok = impl.engine.attach(impl.system.get(), real_w, real_h, cfg.font_engine);
  // EN: Apply initial dp_ratio; idempotent for the default 1.0f.
  // PT: Aplica dp_ratio inicial; idempotente para o padrão 1.0f.
  if (impl.ok) impl.engine.set_dp_ratio(cfg.dp_ratio);
  // EN: A1 (framework-2D) -- arm the physical input route: install the sink WindowGlfw's 5
  //     GLFW callbacks (already registered inside window.create() above, before engine even
  //     existed) feed into. Only after this call does poll_events() -> glfwPollEvents()
  //     actually reach the UI; before it, callbacks fire into an empty sink_ and are dropped
  //     (WindowGlfw::dispatch's null-check), same as before A1 shipped. Captures the raw
  //     Impl* (not `this`/impl_.get() re-read later): App is move-only and the Impl object's
  //     OWN address never changes across a move (only the unique_ptr's stored pointer moves
  //     with it), so this lambda stays valid for the Impl's whole lifetime regardless of how
  //     many times the owning App is moved.
  // PT: A1 (framework-2D) -- arma a rota de input físico: instala o sink que os 5 callbacks
  //     GLFW do WindowGlfw (já registrados dentro de window.create() acima, antes mesmo do
  //     engine existir) alimentam. Só depois desta chamada poll_events() -> glfwPollEvents()
  //     de fato alcança a UI; antes dela, callbacks disparam num sink_ vazio e são descartados
  //     (null-check de WindowGlfw::dispatch), igual antes do A1 existir. Captura o Impl* cru
  //     (não `this`/impl_.get() relido depois): o App é move-only e o próprio endereço do
  //     objeto Impl nunca muda através de um move (só o ponteiro guardado pelo unique_ptr se
  //     move junto), então esta lambda permanece válida pela vida inteira do Impl,
  //     independente de quantas vezes o App dono for movido.
  if (impl.ok) {
    Impl* impl_raw = impl_.get();
    impl_->window.set_event_sink([impl_raw](const UiEvent& ev) {
      if (!impl_raw->ok) return;
      impl_raw->engine.process_event(ev, 0, 0);
    });
  }
  // EN: Seed the resize-tracking cache (AUD-PUB-1 fix) with the SAME real_w/real_h dims
  //     Engine::attach() just used to construct the Rml::Context (D6 above -- NOT cfg.width/
  //     height, which can differ for a Maximized/Fullscreen-born window or under HiDPI).
  //     render() only re-applies set_viewport() when the window size differs from this cache,
  //     so the first frame does not redo the identical SetDimensions() call attach() already
  //     made -- seeding with the wrong (cfg) dims here would make frame 1 immediately re-issue
  //     a redundant (harmless, but pointless) set_viewport() call to the SAME real dims.
  // PT: Semeia o cache de rastreio de resize (fix AUD-PUB-1) com as MESMAS dims real_w/real_h
  //     que Engine::attach() acabou de usar para construir o Rml::Context (D6 acima -- NÃO
  //     cfg.width/height, que pode diferir pra uma janela nascida Maximized/Fullscreen ou sob
  //     HiDPI). render() só reaplica set_viewport() quando o tamanho da janela difere deste
  //     cache, então o 1º frame não repete a mesma chamada SetDimensions() que attach() já fez
  //     -- semear com as dims (cfg) erradas aqui faria o frame 1 reemitir imediatamente uma
  //     chamada set_viewport() redundante (inofensiva, mas inútil) para as MESMAS dims reais.
  impl_->last_render_w = real_w;
  impl_->last_render_h = real_h;
}

App::~App() = default;
App::App(App&&) noexcept = default;
App& App::operator=(App&&) noexcept = default;

bool App::load(const char* rml_path) {
  if (!ready()) return false;
  return impl_->engine.load(rml_path);
}

// ---------------------------------------------------------------------------
// EN: Data-model API — forward to Engine; Engine guards the ordering constraint.
// PT: API de data-model — encaminha ao Engine; Engine enforça a restrição de ordem.
// ---------------------------------------------------------------------------

bool App::create_data_model(const char* name) {
  if (!ready()) return false;
  return impl_->engine.create_data_model(name);
}

bool App::bind_number(const char* key, double initial) {
  if (!ready()) return false;
  return impl_->engine.bind_number(key, initial);
}

bool App::bind_string(const char* key, const char* initial) {
  if (!ready()) return false;
  return impl_->engine.bind_string(key, initial);
}

bool App::bind_bool(const char* key, bool initial) {
  if (!ready()) return false;
  return impl_->engine.bind_bool(key, initial);
}

bool App::bind_list(const char* key) {
  if (!ready()) return false;
  return impl_->engine.bind_list(key);
}

void App::set_number(const char* key, double value) {
  if (!ready()) return;
  impl_->engine.set_number(key, value);
}

void App::set_string(const char* key, const char* value) {
  if (!ready()) return;
  impl_->engine.set_string(key, value);
}

void App::set_bool(const char* key, bool value) {
  if (!ready()) return;
  impl_->engine.set_bool(key, value);
}

void App::set_list(const char* key, const char* const* items, std::size_t count) {
  if (!ready()) return;
  impl_->engine.set_list(key, items, count);
}

void App::set_dp_ratio(float ratio) {
  if (!ready()) return;
  impl_->engine.set_dp_ratio(ratio);
}

void App::set_asset_base_url(const char* url) {
  if (!ready()) return;
  impl_->engine.set_asset_base_url(url);
}

// EN: A4-WINMODES -- thin forwards to WindowGlfw with the App-wide !ready() guard (same pattern
//     as every other method in this file). WindowGlfw::set_mode()/mode() carry the real logic
//     (D4/D7/D8) -- see window_glfw.cpp/.hpp.
// PT: A4-WINMODES -- repasses finos a WindowGlfw com a guarda !ready() de todo o App (mesmo padrão
//     de todo outro método deste arquivo). WindowGlfw::set_mode()/mode() carregam a lógica de
//     fato (D4/D7/D8) -- ver window_glfw.cpp/.hpp.
bool App::set_window_mode(WindowMode mode) {
  if (!ready()) return false;
  return impl_->window.set_mode(mode);
}

WindowMode App::window_mode() const {
  if (!ready()) return WindowMode::Windowed;
  return impl_->window.mode();
}

void App::get_window_size(int& w, int& h) const {
  if (!ready()) {
    w = 0;
    h = 0;
    return;
  }
  impl_->window.size(w, h);
}

// EN: WIN-ICON -- thin forward to WindowGlfw::set_window_icon (window_glfw.hpp/.cpp), same
//     `!ready()` guard every other App method in this family uses.
// PT: WIN-ICON -- repasse fino pro WindowGlfw::set_window_icon (window_glfw.hpp/.cpp), mesma
//     guarda `!ready()` que todo outro método do App nesta família usa.
bool App::set_window_icon(const void* pixels_rgba8, int w, int h) {
  if (!ready()) return false;
  return impl_->window.set_window_icon(pixels_rgba8, w, h);
}

bool App::ready() const noexcept {
  return impl_ && impl_->ok;
}

bool App::ok() const noexcept {
  return ready();
}

bool App::running() const {
  return ready() && !impl_->window.should_close();
}

void App::poll_events() {
  // EN: Guard: consistent with load()/running() — no-op if subsystem init failed.
  // PT: Guard: consistente com load()/running() — no-op se a inicialização falhou.
  if (!ready()) return;
  impl_->window.poll();
}

void App::update() {
  if (ready()) impl_->engine.update();
}

// EN: AUD-PUB-1 fix, extracted as a shared helper (AUD-PUB-1-TWIN, v0.5.0) -- App::render()
//     used to feed (w, h) straight into Engine::render_standalone() without ever calling
//     Engine::set_viewport(), the only path that reaches Rml::Context::SetDimensions()
//     (engine.cpp). The window resized at the GLFW/GL level but the RmlUi layout stayed frozen
//     at the AppConfig construction-time dimensions forever. Fix (Option A, automatic, zero new
//     public API): re-flow the layout whenever the observed size differs from the last frame's
//     cached size, BEFORE rendering with the new size. set_viewport() already guards
//     w<=0||h<=0 (engine.cpp) -- a transiently-zero size (e.g. minimized window) is a harmless
//     no-op there, and the cache is only updated when it actually changes, so a later real size
//     is picked up on the next call.
//     TWIN BUG (found by adversarial review, v0.5.0): App::snapshot() had the EXACT SAME bug --
//     it read window.size() and called render_standalone(w, h) directly, with no
//     sync_viewport() call at all, sharing NEITHER the fix NOR the last_render_w/h cache with
//     render(). A resize with no intervening render() (e.g. host calls only snapshot() in a
//     tight loop, or resize happens between the last render() and a snapshot() call) captured a
//     PPM with the layout frozen at the previous size. Both render() and snapshot() now call
//     this ONE helper so there is a single source of truth for "did the window size change
//     since we last told RmlUi about it" -- impossible for the two call sites to drift again.
// PT: Fix AUD-PUB-1, extraído como helper compartilhado (AUD-PUB-1-TWIN, v0.5.0) --
//     App::render() alimentava (w, h) direto em Engine::render_standalone() sem nunca chamar
//     Engine::set_viewport(), o único caminho que alcança Rml::Context::SetDimensions()
//     (engine.cpp). A janela redimensionava no nível GLFW/GL mas o layout do RmlUi ficava
//     congelado para sempre nas dimensões de construção do AppConfig. Fix (Opção A, automático,
//     zero API pública nova): refaz o layout sempre que o tamanho observado difere do tamanho
//     cacheado do último frame, ANTES de renderizar com o novo tamanho. set_viewport() já
//     guarda w<=0||h<=0 (engine.cpp) -- um tamanho transitoriamente zero (ex.: janela
//     minimizada) é um no-op inofensivo lá, e o cache só é atualizado quando de fato muda,
//     então um tamanho real posterior é capturado na próxima chamada.
//     BUG GÊMEO (achado por review adversarial, v0.5.0): App::snapshot() tinha O MESMO bug
//     EXATO -- lia window.size() e chamava render_standalone(w, h) direto, sem NENHUMA chamada
//     a sync_viewport(), sem compartilhar NEM o fix NEM o cache last_render_w/h com render(). Um
//     resize sem render() no meio (ex.: host chama só snapshot() num loop apertado, ou resize
//     acontece entre o último render() e uma chamada a snapshot()) capturava um PPM com o layout
//     congelado no tamanho anterior. render() e snapshot() agora chamam este ÚNICO helper, então
//     existe uma única fonte de verdade para "o tamanho da janela mudou desde que avisamos o
//     RmlUi" -- impossível os dois call sites voltarem a divergir.
void App::Impl::sync_viewport(int w, int h) {
  if (w != last_render_w || h != last_render_h) {
    engine.set_viewport(w, h);
    last_render_w = w;
    last_render_h = h;
  }
}

// EN: A1 (framework-2D) -- see the doc-comment on this method's declaration (Impl struct
//     above) for why render() and snapshot() BOTH call this instead of each inlining their own
//     copy. Only measures dt / builds the scene_hook lambda when a frame callback is actually
//     installed (zero cost for every App that never calls set_frame_callback(), i.e. every
//     pre-A1 consumer). glfwGetTime() is safe here: app.cpp is already a GLFW-backend-only TU
//     (GLINTFX_BACKEND_GLFW=ON gates its compilation) and transitively includes
//     <GLFW/glfw3.h> via RmlUi_Platform_GLFW.h above.
// PT: A1 (framework-2D) -- ver o doc-comment na declaração deste método (struct Impl acima)
//     pra racional de por que render() E snapshot() chamam este em vez de cada um inlinar a
//     própria cópia. Só mede dt / constrói a lambda scene_hook quando um frame callback está
//     de fato instalado (custo zero para todo App que nunca chama set_frame_callback(), ou
//     seja, todo consumidor pré-A1). glfwGetTime() é seguro aqui: app.cpp já é uma TU
//     exclusiva do backend GLFW (compilação controlada por GLINTFX_BACKEND_GLFW=ON) e inclui
//     <GLFW/glfw3.h> transitivamente via RmlUi_Platform_GLFW.h acima.
void App::Impl::render_frame(int w, int h) {
  if (frame_cb) {
    const double now = glfwGetTime();
    const float  dt  = frame_cb_seen ? static_cast<float>(now - last_frame_time) : 0.0f;
    last_frame_time = now;
    frame_cb_seen   = true;
    engine.render_standalone(w, h, [&]() { frame_cb(dt); });
  } else {
    engine.render_standalone(w, h);
  }
}

void App::render() {
  // EN: Guard: no-op if subsystem init failed (consistent with load()/running()).
  // PT: Guard: no-op se a inicialização falhou (consistente com load()/render()).
  if (!ready()) return;
  int w = 0, h = 0;
  impl_->window.size(w, h);
  impl_->sync_viewport(w, h);
  impl_->render_frame(w, h);
  impl_->window.swap();
}

bool App::snapshot(const char* ppm_path) {
  // EN: Guard: null path is a caller error, not a crash (fopen(NULL) is UB). Twin of the
  //     load(nullptr) guard hardened in v0.3.0 — this is the same class of bug, applied to
  //     the other public const char* path parameter.
  // PT: Guard: caminho nulo é erro do caller, não crash (fopen(NULL) é UB). Gêmeo do guard
  //     de load(nullptr) endurecido na v0.3.0 — mesma classe de bug, aplicada ao outro
  //     parâmetro público const char* de caminho.
  if (!ppm_path) return false;
  // EN: Render one frame, read FBO 0 BEFORE swap (back-buffer has defined content here),
  //     save as PPM, then swap. On many GL implementations (including Mesa/llvmpipe under
  //     Xvfb) the back-buffer content is undefined AFTER glfwSwapBuffers, so capture must
  //     happen here, between EndFrame and the buffer swap.
  // PT: Renderiza um frame, lê o FBO 0 ANTES do swap (back-buffer tem conteúdo definido aqui),
  //     salva como PPM, depois troca. Em muitas implementações GL (incluindo Mesa/llvmpipe sob
  //     Xvfb) o conteúdo do back-buffer é indefinido APÓS glfwSwapBuffers, então a captura
  //     deve acontecer aqui, entre EndFrame e a troca de buffer.
  if (!ready()) return false;
  int w = 0, h = 0;
  impl_->window.size(w, h);
  // EN: AUD-PUB-1-TWIN fix (v0.5.0, found by adversarial review of the AUD-PUB-1 render() fix)
  //     -- snapshot() used to skip this call entirely and go straight to render_standalone(),
  //     capturing a PPM with a layout frozen at whatever size RmlUi last knew about. See the
  //     sync_viewport() doc-comment (above App::render()) for the full history.
  // PT: Fix AUD-PUB-1-TWIN (v0.5.0, achado por review adversarial do fix AUD-PUB-1 de
  //     render()) -- snapshot() pulava esta chamada inteiramente e ia direto pro
  //     render_standalone(), capturando um PPM com layout congelado no último tamanho que o
  //     RmlUi conhecia. Ver o doc-comment de sync_viewport() (acima de App::render()) pra
  //     história completa.
  impl_->sync_viewport(w, h);
  // EN: A1 (framework-2D) -- render_frame() (not a bare render_standalone(w, h) call) so the
  //     frame_cb hook, if installed, fires here too -- see render_frame()'s doc-comment (Impl
  //     struct declaration, above in this file) for why snapshot() sharing this with render()
  //     matters.
  // PT: A1 (framework-2D) -- render_frame() (não uma chamada nua a render_standalone(w, h))
  //     para que o hook frame_cb, se instalado, dispare aqui também -- ver o doc-comment de
  //     render_frame() (declaração da struct Impl, acima neste arquivo) pra por que
  //     compartilhar isto com render() importa.
  impl_->render_frame(w, h);
  // EN: FBO 0 now contains the complete composited frame. Read before swap.
  // PT: FBO 0 agora contém o frame composto completo. Lê antes do swap.
  glBindFramebuffer(0x8D40, 0); // GL_FRAMEBUFFER, 0 = window
  glReadBuffer(0x0402);         // GL_BACK
  // EN: PACK_ALIGNMENT=1: pack rows with no padding so w*3 bytes/row matches vector size exactly.
  // PT: PACK_ALIGNMENT=1: empacota linhas sem padding para que w*3 bytes/linha bata exatamente.
  glPixelStorei(0x0D05, 1);     // GL_PACK_ALIGNMENT = 1
  std::vector<unsigned char> px((size_t)w * h * 3);
  glReadPixels(0, 0, w, h, 0x1907, 0x1401, px.data()); // GL_RGB, GL_UNSIGNED_BYTE
  glPixelStorei(0x0D05, 4);     // restore GL_PACK_ALIGNMENT = 4
  impl_->window.swap();
  FILE* f = fopen(ppm_path, "wb");
  if (!f) return false;
  fprintf(f, "P6\n%d %d\n255\n", w, h);
  // EN: glReadPixels origin is bottom-left; PPM origin is top-left.
  //     Write rows in reverse order (last row first) so the image is upright.
  // PT: A origem do glReadPixels é bottom-left; a do PPM é top-left.
  //     Grava as linhas em ordem inversa (última linha primeiro) para a imagem ficar na vertical correta.
  const size_t row_bytes = (size_t)w * 3;
  for (int row = h - 1; row >= 0; --row) {
    fwrite(px.data() + (size_t)row * row_bytes, 1, row_bytes, f);
  }
  fclose(f);
  return true;
}

App::CapturedFrame App::capture_frame() {
  // EN: `FRAMEGRAB-TEX` -- see the CapturedFrame/capture_frame() doc-comment in app.hpp (right
  //     before `private:`) for the full rationale: form (b) vs (a), pixel format, ownership,
  //     fail-high. This body is deliberately a close structural twin of snapshot() above: same
  //     render_frame() call, same FBO 0 read point (between EndFrame and the buffer swap).
  //     `FRAMEGRAB-EMBED` (v0.27.0) ICED the actual glReadPixels/flip/RGBA8-expansion body
  //     that used to live here into Engine::capture_frame (engine.cpp) -- the SAME readback
  //     UiLayer::capture_frame (ui_layer.cpp) now shares, so the two facades never drift. This
  //     method's own job stays exactly what it always was: drive App's OWN render (App owns
  //     the whole window, offset always (0,0)) and the buffer swap, which UiLayer's read-only
  //     sibling deliberately does NOT do (see UiLayer::capture_frame's own doc-comment,
  //     ui_layer.hpp, for why: the HOST calls render()+capture_frame() itself, in that order,
  //     before its own swap).
  // PT: `FRAMEGRAB-TEX` -- ver o doc-comment de CapturedFrame/capture_frame() em app.hpp (logo
  //     antes de `private:`) pro racional completo: forma (b) vs (a), formato de pixel, posse,
  //     fail-high. Este corpo é de propósito um gêmeo estrutural próximo do snapshot() acima:
  //     mesma chamada render_frame(), mesmo ponto de leitura do FBO 0 (entre o EndFrame e a
  //     troca de buffer). O `FRAMEGRAB-EMBED` (v0.27.0) GELOU o corpo de fato
  //     glReadPixels/flip/expansão-RGBA8 que vivia aqui para dentro de Engine::capture_frame
  //     (engine.cpp) -- o MESMO readback que UiLayer::capture_frame (ui_layer.cpp) agora
  //     compartilha, para que as duas fachadas nunca derivem. O trabalho deste método continua
  //     exatamente o que sempre foi: conduzir o PRÓPRIO render do App (o App é dono da janela
  //     inteira, offset sempre (0,0)) e o swap de buffer, que o irmão só-leitura UiLayer::
  //     capture_frame deliberadamente NÃO faz (ver o próprio doc-comment de
  //     UiLayer::capture_frame, ui_layer.hpp, pro porquê: o HOST chama render()+capture_frame()
  //     ele mesmo, nessa ordem, antes do próprio swap dele).
  if (!ready()) return CapturedFrame{};
  int w = 0, h = 0;
  impl_->window.size(w, h);
  // EN: Defensive fail-high (not reachable through normal App construction, but the same
  //     w<=0||h<=0 shape create_texture()'s own guard documents) -- never call glReadPixels
  //     with a degenerate rectangle.
  // PT: Fail-high defensivo (não alcançável através da construção normal do App, mas a mesma
  //     forma w<=0||h<=0 que o próprio guard do create_texture() documenta) -- nunca chama
  //     glReadPixels com um retângulo degenerado.
  if (w <= 0 || h <= 0) return CapturedFrame{};
  impl_->sync_viewport(w, h);
  impl_->render_frame(w, h);
  // EN: FBO 0 now contains the complete composited frame. Engine::capture_frame reads it
  //     BEFORE the swap below (same rationale as snapshot() above: back-buffer content is
  //     undefined on many GL implementations AFTER glfwSwapBuffers) -- App always reads the
  //     origin-anchored full window, (0, 0).
  // PT: FBO 0 agora contém o frame composto completo. Engine::capture_frame o lê ANTES do
  //     swap abaixo (mesmo racional do snapshot() acima: conteúdo do back-buffer é indefinido
  //     em muitas implementações GL DEPOIS de glfwSwapBuffers) -- o App sempre lê a janela
  //     inteira ancorada na origem, (0, 0).
  CapturedFramePixels px = impl_->engine.capture_frame(0, 0, w, h);
  impl_->window.swap();

  CapturedFrame out;
  out.ok = px.ok;
  out.width = px.width;
  out.height = px.height;
  out.byte_count = px.byte_count;
  out.pixels = std::move(px.pixels);
  return out;
}

void App::process_event(const UiEvent& ev) {
  if (!ready()) return;
  // EN: Resize documented no-op (see app.hpp doc-comment) -- App owns the window and is
  //     already the size authority via render()/snapshot()'s sync_viewport().
  // PT: Resize é no-op documentado (ver doc-comment em app.hpp) -- o App é dono da janela e já
  //     é a própria autoridade sobre o tamanho via sync_viewport() de render()/snapshot().
  if (ev.type == UiEvent::Type::Resize) return;
  impl_->engine.process_event(ev, 0, 0);
}

// EN: HOSTIN-1/2 (Onda 2, v0.19.0) -- thin forwards to WindowGlfw with the App-wide !ready() guard
//     (same pattern as every other method in this file). WindowGlfw carries the real state
//     table + callback plumbing (D2, window_glfw.cpp/.hpp).
// PT: HOSTIN-1/2 (Onda 2, v0.19.0) -- repasses finos a WindowGlfw com a guarda !ready() de todo o
//     App (mesmo padrão de todo outro método deste arquivo). WindowGlfw carrega a tabela de
//     estado + plomberia de callback de fato (D2, window_glfw.cpp/.hpp).
bool App::is_key_down(Key k) const {
  if (!ready()) return false;
  return impl_->window.is_key_down(k);
}

// EN: SEED-SCANCODE (W27) -- same thin-forward pattern as is_key_down() above.
// PT: SEED-SCANCODE (W27) -- mesmo padrão de repasse fino do is_key_down() acima.
bool App::is_scancode_down(int scancode) const {
  if (!ready()) return false;
  return impl_->window.is_scancode_down(scancode);
}

bool App::is_mouse_button_down(int button) const {
  if (!ready()) return false;
  return impl_->window.is_mouse_button_down(button);
}

void App::get_cursor_pos(float& x, float& y) const {
  if (!ready()) {
    x = 0.f;
    y = 0.f;
    return;
  }
  impl_->window.get_cursor_pos(x, y);
}

void App::set_key_callback(std::function<void(Key, KeyAction, int)> cb) {
  if (!ready()) return;
  impl_->window.set_key_callback(std::move(cb));
}

// EN: HOSTIN-3/4/5 (Onda 2, v0.19.0) -- same thin-forward pattern as above.
// PT: HOSTIN-3/4/5 (Onda 2, v0.19.0) -- mesmo padrão de repasse fino de acima.
void App::request_close() {
  if (!ready()) return;
  impl_->window.request_close();
}

void App::set_close_request_callback(std::function<bool()> cb) {
  if (!ready()) return;
  impl_->window.set_close_request_callback(std::move(cb));
}

void App::set_window_focus_callback(std::function<void(bool)> cb) {
  if (!ready()) return;
  impl_->window.set_window_focus_callback(std::move(cb));
}

void App::set_window_iconify_callback(std::function<void(bool)> cb) {
  if (!ready()) return;
  impl_->window.set_window_iconify_callback(std::move(cb));
}

bool App::is_window_focused() const {
  if (!ready()) return false;
  return impl_->window.is_window_focused();
}

bool App::is_window_iconified() const {
  if (!ready()) return false;
  return impl_->window.is_window_iconified();
}

void App::set_frame_callback(std::function<void(float)> cb) {
  if (!ready()) return;
  impl_->frame_cb = std::move(cb);
}

void App::set_click_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_click_callback(std::move(cb));
}

// EN: AUD-PUB-6g -- straight to impl_->system (the SystemInterfaceGlfwDedup this App owns),
//     NOT through Engine/Bootstrap like set_click_callback above: same reasoning as
//     UiLayer::set_cursor_callback (src/ui_layer.cpp) -- the cursor signal originates in
//     Rml::SystemInterface::SetMouseCursor(), intercepted directly by glintfx's own
//     SystemInterface subclass rather than routed through Bootstrap's RmlUi event listeners.
// PT: Direto para impl_->system (o SystemInterfaceGlfwDedup que este App possui), NÃO via
//     Engine/Bootstrap como o set_click_callback acima: mesmo raciocínio de
//     UiLayer::set_cursor_callback (src/ui_layer.cpp) -- o sinal de cursor se origina em
//     Rml::SystemInterface::SetMouseCursor(), interceptado diretamente pela subclasse própria
//     de SystemInterface da glintfx em vez de roteado pelos listeners de evento do RmlUi do
//     Bootstrap.
void App::set_cursor_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->system->set_cursor_callback(std::move(cb));
}

void App::set_click_info_callback(std::function<void(const ClickInfo&)> cb) {
  if (!ready()) return;
  // EN: Straight passthrough (AUD-PUB-4, v0.5.0) -- no coordinate translation needed: App owns
  //     the whole window, so the sub-viewport offset UiLayer::set_click_info_callback adds is
  //     always (0,0) here (same reasoning as App::get_element_box's lack of offset addition).
  // PT: Repasse direto (AUD-PUB-4, v0.5.0) -- sem tradução de coordenada necessária: o App é
  //     dono da janela inteira, então o offset de sub-viewport que
  //     UiLayer::set_click_info_callback soma é sempre (0,0) aqui (mesma racional da ausência
  //     de soma de offset em App::get_element_box).
  impl_->engine.set_click_info_callback(std::move(cb));
}

void App::set_scroll_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  // EN: Straight passthrough (GLINTFX-SCROLL-1 follow-up, v0.6.0) -- id-only, no coordinate
  //     translation applicable (there is none for scroll, unlike ClickInfo's x/y).
  // PT: Repasse direto (desdobramento do GLINTFX-SCROLL-1, v0.6.0) -- só-id, sem tradução de
  //     coordenada aplicável (não existe uma pro scroll, diferente do x/y do ClickInfo).
  impl_->engine.set_scroll_callback(std::move(cb));
}

// EN: L1.15-FORMEV -- straight passthrough, no coordinate translation (App owns the whole
//     window; none of these five payloads carry geometry -- see the doc-comments in app.hpp/
//     bootstrap.hpp for the full contract).
// PT: L1.15-FORMEV -- repasse direto, sem tradução de coordenada (o App é dono da janela
//     inteira; nenhum dos cinco payloads carrega geometria -- ver os doc-comments em app.hpp/
//     bootstrap.hpp para o contrato completo).
void App::set_change_callback(std::function<void(const char*, const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_change_callback(std::move(cb));
}

void App::set_submit_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_submit_callback(std::move(cb));
}

void App::set_focus_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_focus_callback(std::move(cb));
}

void App::set_blur_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_blur_callback(std::move(cb));
}

void App::set_hover_callback(std::function<void(const char*, bool)> cb) {
  if (!ready()) return;
  impl_->engine.set_hover_callback(std::move(cb));
}

ElementBox App::get_element_box(const char* id) const {
  ElementBox box;
  if (!ready()) return box;
  float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
  if (impl_->engine.get_element_box(id, x, y, w, h)) {
    box.found = true; box.x = x; box.y = y; box.w = w; box.h = h;
  }
  return box;
}

bool App::scroll_element_into_view(const char* id, bool align_with_top) const {
  if (!ready()) return false;
  return impl_->engine.scroll_element_into_view(id, align_with_top);
}

bool App::scroll_element_into_view(const char* id, ScrollAlign align) const {
  if (!ready()) return false;
  return impl_->engine.scroll_element_into_view(id, align);
}

bool App::get_element_scroll_top(const char* id, float& out_scroll_top) const {
  if (!ready()) return false;
  return impl_->engine.get_element_scroll_top(id, out_scroll_top);
}

bool App::get_element_scroll_height(const char* id, float& out_scroll_height) const {
  if (!ready()) return false;
  return impl_->engine.get_element_scroll_height(id, out_scroll_height);
}

bool App::get_element_client_height(const char* id, float& out_client_height) const {
  if (!ready()) return false;
  return impl_->engine.get_element_client_height(id, out_client_height);
}

bool App::set_element_scroll_top(const char* id, float scroll_top) const {
  if (!ready()) return false;
  return impl_->engine.set_element_scroll_top(id, scroll_top);
}

bool App::set_focus(const char* id) const {
  if (!ready()) return false;
  return impl_->engine.set_focus(id);
}

bool App::clear_focus() const {
  if (!ready()) return false;
  return impl_->engine.clear_focus();
}

// ---------------------------------------------------------------------------
// EN: DOM read/write by id (L1.16-DOMRW) -- straight forwards to Engine.
// PT: Leitura/escrita de DOM por id (L1.16-DOMRW) -- repasse direto ao Engine.
// ---------------------------------------------------------------------------

bool App::set_text(const char* id, const char* text) const {
  if (!ready()) return false;
  return impl_->engine.set_text(id, text);
}

bool App::add_class(const char* id, const char* cls) const {
  if (!ready()) return false;
  return impl_->engine.add_class(id, cls);
}

bool App::remove_class(const char* id, const char* cls) const {
  if (!ready()) return false;
  return impl_->engine.remove_class(id, cls);
}

bool App::set_property(const char* id, const char* prop, const char* value) const {
  if (!ready()) return false;
  return impl_->engine.set_property(id, prop, value);
}

bool App::load_font_face(const FontFaceDesc& desc) {
  if (!ready()) return false;
  return impl_->engine.load_font_face(desc);
}

bool App::get_number(const char* key, double& out) const {
  if (!ready()) return false;
  return impl_->engine.get_number(key, out);
}

bool App::get_string(const char* key, std::string& out) const {
  if (!ready()) return false;
  return impl_->engine.get_string(key, out);
}

bool App::get_bool(const char* key, bool& out) const {
  if (!ready()) return false;
  return impl_->engine.get_bool(key, out);
}

// EN: WM-VSYNC -- thin forwards to WindowGlfw with the App-wide !ready() guard (same pattern as
//     every other method in this file). WindowGlfw::set_swap_interval/get_monitor_refresh_hz
//     carry the real logic (window_glfw.cpp/.hpp).
// PT: WM-VSYNC -- repasses finos a WindowGlfw com a guarda !ready() de todo o App (mesmo padrão de
//     todo outro método deste arquivo). WindowGlfw::set_swap_interval/get_monitor_refresh_hz
//     carregam a lógica de fato (window_glfw.cpp/.hpp).
bool App::set_swap_interval(int interval) {
  if (!ready()) return false;
  return impl_->window.set_swap_interval(interval);
}

bool App::set_vsync(bool enabled) {
  return set_swap_interval(enabled ? 1 : 0);
}

int App::get_monitor_refresh_hz() const {
  if (!ready()) return 0;
  return impl_->window.get_monitor_refresh_hz();
}

} // namespace glintfx

// EN: APP-MINIMIZED (W22, S5) -- glfw_decide_wait_for_events, the pure decision seam run()
//     below calls into. See that function's own doc-comment (glfw_event_translate.hpp) for
//     the full "why glfwWaitEvents(), not sleep" rationale. The namespace above is closed and
//     reopened around this #include ON PURPOSE, instead of grouping it with the top-of-file
//     includes (window_glfw.cpp's own pattern) or #include-ing it directly inside the
//     still-open `namespace glintfx { ... }` this file uses for everything above: the header
//     opens ITS OWN `namespace glintfx { ... }`, and splicing that in while already inside
//     this file's outer `namespace glintfx` nests it into `glintfx::glintfx` -- the compiler
//     then cannot resolve `glfw_decide_wait_for_events` from inside run() below (confirmed by
//     hand: it suggested the fully-qualified `::glintfx::glintfx::glfw_decide_wait_for_events`
//     right back). Moving the #include to the true top of the file (before line 33's
//     `namespace glintfx {`) would fix the nesting but shifts every app.cpp:N line
//     docs/embed-integration.md cites (app.cpp:476 for capture_frame(), among others -- see
//     AGENTS.md's doc-citation discipline) -- this file's own namespace block is closed and
//     reopened here, immediately above run() (the LAST function in this translation unit,
//     after every cited line), so nothing above this point moves.
// PT: APP-MINIMIZED (W22, S5) -- glfw_decide_wait_for_events, o seam de decisão pura que o
//     run() abaixo chama. Ver o próprio doc-comment daquela função (glfw_event_translate.hpp)
//     pra racional completa de "por que glfwWaitEvents(), não sleep". O namespace acima é
//     fechado e reaberto ao redor deste #include DE PROPÓSITO, em vez de agrupá-lo com os
//     includes do topo do arquivo (o próprio padrão do window_glfw.cpp) ou incluí-lo direto
//     dentro do `namespace glintfx { ... }` ainda aberto que este arquivo usa para tudo acima:
//     o header abre O PRÓPRIO `namespace glintfx { ... }`, e emendar isso enquanto já se está
//     dentro do `namespace glintfx` externo deste arquivo aninha em `glintfx::glintfx` -- o
//     compilador então não consegue resolver `glfw_decide_wait_for_events` de dentro do run()
//     abaixo (confirmado na mão: ele sugeriu de volta o nome totalmente qualificado
//     `::glintfx::glintfx::glfw_decide_wait_for_events`). Mover o #include pro topo de fato do
//     arquivo (antes do `namespace glintfx {` da linha 33) resolveria o aninhamento mas
//     deslocaria toda linha app.cpp:N que o docs/embed-integration.md cita (app.cpp:476 pro
//     capture_frame(), entre outras -- ver a disciplina de citação de doc do AGENTS.md) --
//     o namespace deste arquivo é fechado e reaberto aqui, logo acima do run() (a ÚLTIMA
//     função desta unidade de tradução, depois de toda linha citada), então nada acima deste
//     ponto se move.
#include "glfw_event_translate.hpp"

namespace glintfx {

void App::run() {
  while (running()) {
    // EN: APP-MINIMIZED (W22, S5) -- BUG FIX: this loop used to be an unconditional
    //     poll_events()+update()+render(), which burns CPU when the window's framebuffer is
    //     degenerate (commonly 0x0 under a Wayland-minimized window -- glfwGetFramebufferSize,
    //     read below via the SAME impl_->window.size() render()/snapshot() already use). No
    //     framebuffer means no swap, no swap means vsync provides ZERO pacing, so the naive
    //     loop spun at thousands of iterations/second. Found because the GusWorld consumer
    //     asked "does your own App::run() loop have this problem too?" while explaining its
    //     own 3 SDL_Delay(16) guards -- it did (see TODO.md's APP-MINIMIZED entry for the full
    //     story). Decision (CTO): degenerate framebuffer -> glfwWaitEvents(), never a sleep --
    //     see glfw_decide_wait_for_events's doc-comment (glfw_event_translate.hpp) for the full
    //     "wakes exactly on restore, not on a clock guess" rationale. glfwWaitEvents() itself
    //     blocks-and-drains the event queue (same effect poll_events() has on the events it
    //     processes), so poll_events()/update()/render() are skipped entirely for this pass --
    //     calling poll_events() too would be redundant, not merely harmless.
    // PT: APP-MINIMIZED (W22, S5) -- CONSERTO DE BUG: este laço costumava ser um
    //     poll_events()+update()+render() incondicional, que queima CPU quando o framebuffer
    //     da janela está degenerado (comumente 0x0 sob uma janela minimizada no Wayland --
    //     glfwGetFramebufferSize, lido abaixo via o MESMO impl_->window.size() que
    //     render()/snapshot() já usam). Sem framebuffer não há swap, sem swap o vsync não dá
    //     ritmo NENHUM, então o laço ingênuo girava a milhares de iterações por segundo.
    //     Achado porque o consumidor GusWorld perguntou "o laço App::run() de vocês tem esse
    //     mesmo problema?" ao explicar as 3 guardas SDL_Delay(16) próprias -- tinha (ver a
    //     entrada APP-MINIMIZED do TODO.md pra história completa). Decisão (CTO): framebuffer
    //     degenerado -> glfwWaitEvents(), nunca um sleep -- ver o doc-comment de
    //     glfw_decide_wait_for_events (glfw_event_translate.hpp) pra racional completa de
    //     "acorda exatamente no restaurar, não por um chute de relógio". O próprio
    //     glfwWaitEvents() bloqueia-e-drena a fila de eventos (o mesmo efeito que
    //     poll_events() tem sobre os eventos que processa), então poll_events()/update()/
    //     render() são pulados inteiramente neste passo -- chamar poll_events() também seria
    //     redundante, não só inofensivo.
    int w = 0, h = 0;
    impl_->window.size(w, h);
    if (glfw_decide_wait_for_events(w, h)) {
      glfwWaitEvents();
      continue;
    }
    poll_events();
    update();
    render();
  }
}

} // namespace glintfx
