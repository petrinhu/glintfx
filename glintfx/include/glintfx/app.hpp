// SPDX-License-Identifier: MPL-2.0
// glintfx — drop-in UI engine + GL3 effects facade.
// Fachada drop-in: motor de UI + efeitos GL3.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <glintfx/config.hpp>
#if !GLINTFX_BACKEND_GLFW
#error "glintfx::App requires GLINTFX_BACKEND_GLFW=ON (embed-only build: use glintfx::UiLayer)"
#endif
#include <memory>
#include <cstddef>
#include <functional>
#include <string>  // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).
// EN: version() moved to its own header (L1.9-VERSEMBED) so embed-only consumers that
//     never include app.hpp still get glintfx::version(). Re-included here (transitively)
//     so existing code that only includes app.hpp keeps compiling unchanged.
// PT: version() foi movida para header próprio (L1.9-VERSEMBED) para que consumidores
//     embed-only, que nunca incluem app.hpp, ainda tenham glintfx::version(). Reincluída
//     aqui (transitivamente) para que código existente que só inclui app.hpp continue
//     compilando sem mudanças.
#include <glintfx/version.hpp>
#include <glintfx/element_box.hpp>
#include <glintfx/click_info.hpp>
#include <glintfx/font_engine.hpp>
#include <glintfx/window_mode.hpp>  // EN: window mode (A4-WINMODES, framework-2D). PT: modo de janela (A4-WINMODES, framework-2D).
#include <glintfx/ui_event.hpp>  // EN: process_event (A1, framework-2D). PT: process_event (A1, framework-2D).
namespace glintfx {

// EN: Configuration for App construction. Zero-initialize safe; defaults are sane.
// PT: Configuração para construção do App. Zero-init seguro; defaults razoáveis.
struct AppConfig {
  const char* title = "glintfx";
  int width  = 1280;
  int height = 720;
  // EN: Initial density-independent pixel ratio. 1 dp = dp_ratio physical pixels.
  //     Matches UiLayerConfig::dp_ratio; see set_dp_ratio() for runtime update.
  // PT: Density-independent pixel ratio inicial. 1 dp = dp_ratio pixels físicos.
  //     Corresponde a UiLayerConfig::dp_ratio; ver set_dp_ratio() para update em runtime.
  float dp_ratio = 1.0f;
  // EN: Which font engine to install (L1.20-FONTFLIP Phase 2). Fixed at construction --
  //     App reads this exactly once, during the ctor's Bootstrap::init() call, before
  //     Rml::Initialise(); there is no runtime setter (see FontEngine's own doc-comment for the
  //     full contract, including the GLINTFX_OWN_FONT_ENGINE=OFF fallback-to-FreeType path).
  //     Default: FontEngine::Own -- the "soft flip" (L1.20-FONTFLIP Phase 1) makes glintfx's own
  //     clean-room font engine the default; set to FontEngine::FreeType for a one-line,
  //     no-rebuild rollback to RmlUi's built-in engine.
  // PT: Qual motor de fonte instalar (L1.20-FONTFLIP Fase 2). Fixado na construção -- o App lê
  //     isto exatamente uma vez, durante a chamada a Bootstrap::init() do ctor, antes do
  //     Rml::Initialise(); não há setter de runtime (ver o doc-comment do próprio FontEngine
  //     para o contrato completo, incluindo o caminho de fallback-para-FreeType quando
  //     GLINTFX_OWN_FONT_ENGINE=OFF). Default: FontEngine::Own -- o "flip suave"
  //     (L1.20-FONTFLIP Fase 1) torna o motor de fonte próprio clean-room do glintfx o padrão;
  //     defina FontEngine::FreeType para um rollback de uma linha, sem rebuild, ao motor
  //     embutido do RmlUi.
  FontEngine font_engine = FontEngine::Own;

  // EN: Initial window mode (A4-WINMODES, framework-2D; consumer-driven -- GusWorld's maximized
  //     window was invading the KDE taskbar). An invalid value (e.g. an out-of-range
  //     static_cast) falls back to WindowMode::Windowed -- validated by App's constructor
  //     BEFORE the window is created, so create() never fails because of this field. See
  //     window_mode.hpp for the per-mode semantics (Maximized respects the WM work area;
  //     FullscreenDesktop covers everything, panels included, BY DESIGN) and
  //     App::set_window_mode() below for the runtime equivalent.
  // PT: Modo inicial da janela (A4-WINMODES, framework-2D; consumer-driven -- a janela
  //     maximizada do GusWorld invadia a barra de tarefas do KDE). Um valor inválido (ex.: um
  //     static_cast fora de faixa) cai pra WindowMode::Windowed -- validado pelo construtor do
  //     App ANTES da janela ser criada, então create() nunca falha por causa deste campo. Ver
  //     window_mode.hpp para a semântica por modo (Maximized respeita a work area do WM;
  //     FullscreenDesktop cobre tudo, painéis inclusive, BY DESIGN) e App::set_window_mode()
  //     abaixo para o equivalente em runtime.
  WindowMode window_mode = WindowMode::Windowed;
};

// EN: RAII application facade. Owns the window, renderer, and UI bootstrap.
//     Move-only. No third-party engine or graphics types appear in this header.
//
//     GLOBAL STATE (N3): glintfx initialises process-global state (window backend + UI library).
//     Only ONE App instance per process is supported. Creating a second instance while
//     the first is alive, or after it has been destroyed, results in undefined behaviour.
//
//     MOVED-FROM STATE (L1.10-APIDOC): after `App b = std::move(a);` (or `b = std::move(a);`),
//     `a` is left in a moved-from state. Calling ANY method on `a` other than ok() or the
//     destructor is undefined behaviour — same convention as std::unique_ptr. ok() on a
//     moved-from App returns false (impl_ is null), which is the only state query that is
//     safe to make; destruction of a moved-from App is always safe (no-op, impl_ is null).
//
// PT: Fachada RAII da aplicação. Possui janela, renderer e bootstrap de UI.
//     Move-only. Nenhum tipo de engine ou gráficos de terceiros aparece neste header.
//
//     ESTADO GLOBAL (N3): glintfx inicializa estado global de processo (backend de janela + lib de UI).
//     Apenas UMA instância de App por processo é suportada. Criar uma segunda instância
//     enquanto a primeira está viva, ou após ser destruída, resulta em comportamento indefinido.
//
//     ESTADO MOVIDO-DE (L1.10-APIDOC): após `App b = std::move(a);` (ou `b = std::move(a);`),
//     `a` fica em estado movido-de. Chamar QUALQUER método em `a` além de ok() ou o destrutor
//     é comportamento indefinido — mesma convenção do std::unique_ptr. ok() num App movido-de
//     retorna false (impl_ é nulo), sendo a única consulta de estado segura; destruir um App
//     movido-de é sempre seguro (no-op, impl_ é nulo).
class App {
public:
  // EN: Constructs the App: opens a window and initialises the GL context and UI engine.
  //     Construction can fail silently (e.g. no display, no GL context available).
  //     On failure (N2): ok() returns false; running() also returns false; load() is a no-op;
  //     render() and run() do nothing. Check ok() once after construction to distinguish an
  //     initialization failure from a window that was closed after a successful start.
  //     run() is safe to call regardless — it returns immediately when !running().
  // PT: Constrói o App: abre a janela e inicializa o contexto GL e o motor de UI.
  //     A construção pode falhar silenciosamente (ex.: sem display, sem contexto GL disponível).
  //     Em falha (N2): ok() retorna false; running() também retorna false; load() é no-op;
  //     render() e run() não fazem nada. Verifique ok() uma vez após a construção para distinguir
  //     falha de inicialização de uma janela fechada após início bem-sucedido.
  //     run() é seguro de chamar independentemente — retorna imediatamente quando !running().
  explicit App(AppConfig cfg = {});
  ~App();
  // EN: Move leaves the source in a moved-from state — see the class-level comment above
  //     for the exact contract (only ok() and ~App() remain valid on the source afterwards).
  // PT: O move deixa a origem em estado movido-de — ver o comentário de nível de classe
  //     acima para o contrato exato (só ok() e ~App() permanecem válidos na origem depois).
  App(App&&) noexcept;
  App& operator=(App&&) noexcept;
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // EN: Returns true if construction succeeded: window, GL context, and UI engine are all live.
  //     Use ok() to distinguish "initialization failed" from "window closed":
  //     running() returns false in both cases, conflating them. Check ok() once right after
  //     construction; it never changes after that point.
  // PT: Retorna true se a construção teve sucesso: janela, contexto GL e motor de UI estão todos vivos.
  //     Use ok() para distinguir "falha de inicialização" de "janela fechada":
  //     running() retorna false nos dois casos, misturando-os. Verifique ok() uma vez logo após
  //     a construção; ele nunca muda depois desse ponto.
  bool ok() const noexcept;

  // EN: Load an .rml document and show it. Returns true on success.
  //     QW-RIPPLEAPP (v0.18.1): a `decorator: ripple(...)` (ADR-0012) used in a document loaded
  //     by a standalone App is visually INERT BY DESIGN -- ripple() refracts the HOST's own
  //     backdrop, and a standalone App has no host underneath it (it owns the window and its own
  //     FBO 0 contents outright, nothing to refract). The effect only does something in
  //     embed/UiLayer mode, where a real host has already drawn its own scene into FBO 0 before
  //     handing control over. A one-time warning is logged the first time such a decorator
  //     compiles in a standalone App, so this is silent-but-diagnosed, never silent-and-mysterious.
  // PT: Carrega um documento .rml e exibe. Retorna true em caso de sucesso.
  //     QW-RIPPLEAPP (v0.18.1): um `decorator: ripple(...)` (ADR-0012) usado num documento
  //     carregado por um App standalone é INERTE POR DESIGN -- ripple() refrata o próprio
  //     backdrop do HOST, e um App standalone não tem host embaixo dele (é dono da janela e do
  //     próprio conteúdo do FBO 0, nada para refratar). O efeito só faz algo em modo embed/
  //     UiLayer, onde um host real já desenhou a própria cena no FBO 0 antes de entregar o
  //     controle. Um aviso único é logado na primeira vez que tal decorator compila num App
  //     standalone, então isto é silencioso-mas-diagnosticado, nunca silencioso-e-misterioso.
  bool load(const char* rml_path);

  // EN: Update the density-independent pixel ratio at runtime (parity with UiLayer).
  //     Triggers a layout re-pass when changed.
  // PT: Atualiza o density-independent pixel ratio em runtime (paridade com UiLayer).
  //     Dispara re-layout quando alterado.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL for asset resolution (parity with UiLayer).
  //     Relative asset paths are prefixed with base_url/. Call before load().
  // PT: Sobrepõe o base URL para resolução de assets (paridade com UiLayer).
  //     Caminhos relativos de assets são prefixados com base_url/. Chame antes de load().
  void set_asset_base_url(const char* url);

  // EN: Switch window mode at runtime (A4-WINMODES, framework-2D; D1 -- table-stakes for a
  //     game's options menu / Alt+Enter fullscreen toggle). Returns false (no-op, window
  //     untouched) when !ok() or `mode` is not a valid enumerator (an out-of-range
  //     static_cast); true when the request was issued to the window system -- the WM/
  //     compositor has final say, query window_mode() for the LIVE result afterwards.
  //     Re-requesting the current mode is a safe no-op returning true.
  //     Transition mechanics (D7): leaving Windowed saves the current position+size so a later
  //     return to Windowed restores it (a window that was never windowed restores to
  //     AppConfig's construction width×height at position (64,64) -- documented default).
  //     Fullscreen -> Maximized restores windowed FIRST, then maximizes (D8, mandatory order --
  //     GLFW has no direct fullscreen->maximized transition). LATENCY: every successful call
  //     blocks the caller for up to ~150ms before returning (a bounded, non-blind wait -- see
  //     WindowGlfw::set_mode()'s doc-comment, window_glfw.hpp, for the compositor round-trip
  //     this closes); Fullscreen->Maximized blocks up to ~300ms (two requests to settle).
  //     Deliberate for a rare, user-initiated action (options menu, Alt+Enter), not a per-frame
  //     call.
  //     Wayland (D10): FullscreenExclusive is compositor-EMULATED (no real mode-set, the
  //     compositor scales -- GLFW's documented behaviour); restore position is best-effort
  //     (window position is not readable/settable under native Wayland). Primary monitor only
  //     in this slice (multi-monitor selection -- INBOX). See docs/window-modes.md for the
  //     full per-platform breakdown.
  // PT: Troca o modo de janela em runtime (A4-WINMODES, framework-2D; D1 -- table-stakes pra
  //     menu de opções de jogo / toggle de fullscreen Alt+Enter). Retorna false (no-op, janela
  //     intocada) quando !ok() ou `mode` não é um enumerador válido (um static_cast fora de
  //     faixa); true quando o pedido foi emitido ao sistema de janelas -- o WM/compositor tem a
  //     palavra final, consulte window_mode() pelo resultado VIVO depois. Repedir o modo
  //     corrente é um no-op seguro que retorna true.
  //     Mecânica de transição (D7): sair de Windowed salva a posição+tamanho corrente para que
  //     um retorno posterior a Windowed os restaure (uma janela que nunca foi windowed restaura
  //     pro width×height de construção do AppConfig na posição (64,64) -- default documentado).
  //     Fullscreen -> Maximized restaura windowed PRIMEIRO, depois maximiza (D8, ordem
  //     obrigatória -- o GLFW não tem transição direta fullscreen->maximizada). LATÊNCIA: toda
  //     chamada bem-sucedida bloqueia o chamador por até ~150ms antes de retornar (uma espera
  //     limitada, não cega -- ver o doc-comment de WindowGlfw::set_mode(), window_glfw.hpp, pra
  //     ida-e-volta com o compositor que isto fecha); Fullscreen->Maximized bloqueia até ~300ms
  //     (dois pedidos pra assentar). Deliberado pra uma ação rara, iniciada pelo usuário (menu
  //     de opções, Alt+Enter), não uma chamada por-frame.
  //     Wayland (D10): FullscreenExclusive é EMULADO pelo compositor (sem troca de modo real,
  //     o compositor escala -- comportamento documentado do GLFW); a restauração de posição é
  //     best-effort (posição de janela não é legível/gravável sob Wayland nativo). Só monitor
  //     primário nesta fatia (seleção multi-monitor -- INBOX). Ver docs/window-modes.md para o
  //     detalhamento completo por plataforma.
  bool set_window_mode(WindowMode mode);

  // EN: Query the LIVE window mode, derived from the window system -- NOT an echo of the last
  //     set_window_mode()/AppConfig::window_mode request (A4-WINMODES, D4). Returns
  //     WindowMode::Windowed when !ok(). The user (or the WM) can maximize/restore the window
  //     through means glintfx never sees (a taskbar button, a WM keybinding); an echo would
  //     lie in that case. Headless consequence (documented, not a bug): under Xvfb (no window
  //     manager) a Maximized request may not be honoured, and this call can report Windowed --
  //     query it after a render()/poll_events() cycle if the live value matters.
  // PT: Consulta o modo de janela VIVO, derivado do sistema de janelas -- NÃO um eco do último
  //     pedido set_window_mode()/AppConfig::window_mode (A4-WINMODES, D4). Retorna
  //     WindowMode::Windowed quando !ok(). O usuário (ou o WM) pode maximizar/restaurar a
  //     janela por meios que a glintfx nunca vê (um botão da barra de tarefas, um atalho do
  //     WM); um eco mentiria nesse caso. Consequência headless (documentada, não é bug): sob
  //     Xvfb (sem window manager) um pedido Maximized pode não ser honrado, e esta chamada pode
  //     reportar Windowed -- consulte após um ciclo de render()/poll_events() se o valor vivo
  //     importar.
  WindowMode window_mode() const;

  // EN: Query the current framebuffer size, in physical pixels -- the same size render()/
  //     snapshot() compose at (A4-WINMODES). w=h=0 when !ok().
  // PT: Consulta o tamanho corrente do framebuffer, em pixels físicos -- o mesmo tamanho em
  //     que render()/snapshot() compõem (A4-WINMODES). w=h=0 quando !ok().
  void get_window_size(int& w, int& h) const;

  // EN: Returns false if the window was closed or initialization failed.
  //     To distinguish the two cases check ok() once after construction.
  // PT: Retorna false se a janela foi fechada ou se a inicialização falhou.
  //     Para distinguir os dois casos, verifique ok() uma vez após a construção.
  bool running() const;

  // EN: Process pending window/input events (non-blocking).
  // PT: Processa eventos de janela/entrada pendentes (sem bloqueio).
  void poll_events();

  // EN: Advance the UI context by one tick.
  // PT: Avança o contexto de UI por um tick.
  void update();

  // EN: Render one frame (begin_frame → UI render pass → end_frame → swap).
  // PT: Renderiza um frame (begin_frame → passe de render de UI → end_frame → swap).
  void render();

  // EN: Convenience loop: poll + update + render until !running() (blocking; returns only
  //     after the window closes or ok() turns false). This is exactly the composition of
  //     poll_events() -> update() -> render() already exercised frame-by-frame by every other
  //     App test in this suite (app_smoke, data_model_*, app_resize_smoke, etc.) -- run()
  //     itself has no test of its own because a blocking loop cannot be asserted on without
  //     either an external quit signal or a timeout race, and the 3 calls it composes are
  //     already covered individually. AUD-TEC-7 (2026-07-08).
  // PT: Laço de conveniência: poll + update + render até !running() (bloqueante; só retorna
  //     depois que a janela fecha ou ok() vira false). É exatamente a composição de
  //     poll_events() -> update() -> render() já exercitada frame-a-frame por todo outro
  //     teste de App desta suíte (app_smoke, data_model_*, app_resize_smoke, etc.) -- run()
  //     em si não tem teste próprio porque um laço bloqueante não dá pra verificar sem um
  //     sinal externo de saída ou uma corrida de timeout, e as 3 chamadas que ele compõe já
  //     estão cobertas individualmente. AUD-TEC-7 (2026-07-08).
  void run();

  // EN: Inject a synthetic input event (A1, framework-2D) -- the SAME neutral route the embed
  //     mode has used since UiLayer shipped (Engine::process_event, shared verbatim -- see
  //     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md section 2.1). PHYSICAL window
  //     input (keyboard/mouse) already arrives automatically once the window has focus -- GLFW
  //     callbacks translate it and route it through this exact same Engine call (see
  //     window_glfw.cpp / glfw_event_translate.hpp); this method is the synthetic/replay/test
  //     channel, parity with UiLayer::process_event(const UiEvent&) (same signature/contract).
  //     `UiEvent::Type::Resize` is a documented NO-OP here: unlike UiLayer (an embed guest that
  //     does not own the window and must be told its viewport explicitly), App owns the whole
  //     window and is already the authority on its own size via the AUD-PUB-1 sync_viewport
  //     machinery in render()/snapshot() -- injecting a synthetic Resize event here would just
  //     be overwritten by the next render() call reading the real window size. No-op (does
  //     nothing, does not crash) when !ok().
  // PT: Injeta um evento de input sintético (A1, framework-2D) -- a MESMA rota neutra que o
  //     embed mode usa desde que o UiLayer foi lançado (Engine::process_event, compartilhado
  //     tal-e-qual -- ver docs/superpowers/plans/2026-07-19-framework2d-A1-input.md seção 2.1).
  //     Input FÍSICO de janela (teclado/mouse) já chega automaticamente assim que a janela tem
  //     foco -- callbacks GLFW o traduzem e o roteiam pela mesma chamada de Engine exata (ver
  //     window_glfw.cpp / glfw_event_translate.hpp); este método é o canal sintético/replay/
  //     teste, paridade com UiLayer::process_event(const UiEvent&) (mesma assinatura/contrato).
  //     `UiEvent::Type::Resize` é um NO-OP documentado aqui: diferente do UiLayer (um convidado
  //     embed que não é dono da janela e precisa ser avisado do próprio viewport
  //     explicitamente), o App é dono da janela inteira e já é a própria autoridade sobre o
  //     próprio tamanho via a maquinaria AUD-PUB-1 sync_viewport em render()/snapshot() --
  //     injetar um evento Resize sintético aqui seria só sobrescrito pela próxima chamada a
  //     render() lendo o tamanho real da janela. No-op (não faz nada, não crasha) quando !ok().
  void process_event(const UiEvent& ev);

  // EN: Register the game's per-frame draw hook (A1, framework-2D). Called once per
  //     render()/run() iteration -- with the GL context current, AFTER the frame clear and
  //     BEFORE the UI render pass, so a scene drawn inside `cb` composes UNDER the UI -- see
  //     Engine::render_standalone's doc-comment (engine.hpp) for the exact insertion point and
  //     the GL-state contract (the hook may leave arbitrary GL state behind; it is
  //     automatically restored to the RmlUi-established baseline before the UI render pass
  //     runs, so `cb` never needs to save/restore anything itself). `dt_seconds` is the time
  //     elapsed since the PREVIOUS call to this hook (0.0f on the very first call -- there is
  //     no meaningful previous frame to measure against). Passing an empty/null `cb` disables
  //     the hook (no-op per frame, same cost as before this method was ever called). `run()`
  //     is exactly the game's frame loop once this is set: `poll_events() -> update() ->
  //     render()` (the hook fires inside render()), with no change to run()'s own signature.
  // PT: Registra o hook de desenho por-frame do jogo (A1, framework-2D). Chamado uma vez por
  //     iteração de render()/run() -- com o contexto GL corrente, APÓS o clear do frame e ANTES
  //     do passe de render de UI, para que uma cena desenhada dentro de `cb` componha SOB a UI
  //     -- ver o doc-comment de Engine::render_standalone (engine.hpp) para o ponto de inserção
  //     exato e o contrato de estado GL (o hook pode deixar estado GL arbitrário para trás; ele
  //     é automaticamente restaurado à base estabelecida pelo RmlUi antes do passe de render de
  //     UI rodar, então `cb` nunca precisa salvar/restaurar nada por conta própria).
  //     `dt_seconds` é o tempo decorrido desde a chamada ANTERIOR a este hook (0.0f na primeira
  //     chamada -- não há um frame anterior significativo para medir contra). Passar um `cb`
  //     vazio/nulo desativa o hook (no-op por frame, mesmo custo de antes deste método ter sido
  //     chamado alguma vez). run() vira exatamente o loop de frame do jogo assim que isto está
  //     definido: `poll_events() -> update() -> render()` (o hook dispara dentro do render()),
  //     sem mudança na própria assinatura de run().
  void set_frame_callback(std::function<void(float dt_seconds)> cb);

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none reached before the document root).
  //     No ordering constraint versus load(): safe to call before or after it. Parity with
  //     UiLayer::set_click_callback (same signature).
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum for encontrado até a raiz do
  //     documento). Sem restrição de ordem vs. load(): seguro chamar antes ou depois. Paridade
  //     com UiLayer::set_click_callback (mesma assinatura).
  void set_click_callback(std::function<void(const char* element_id)> cb);

  // EN: Register a RICHER click callback -- id + button + coordinates + double-click, a
  //     second/parallel channel alongside set_click_callback (AUD-PUB-4, v0.5.0). Does NOT
  //     replace set_click_callback -- both can be registered simultaneously and both fire
  //     independently on the same click (see ClickInfo's doc-comment for the full contract,
  //     including a double-click firing this callback 3 times total, and the `button` field's
  //     known limitation: effectively always 0, since RmlUi never dispatches Click/Dblclick for
  //     non-primary mouse buttons). Coordinate space: window physical pixels, top-left origin,
  //     y-down -- App owns the whole window, so there is no sub-viewport offset to translate
  //     (unlike UiLayer). Parity with UiLayer::set_click_info_callback (same signature).
  // PT: Registra um callback de clique MAIS RICO -- id + botão + coordenadas + duplo-clique,
  //     um segundo canal/paralelo ao set_click_callback (AUD-PUB-4, v0.5.0). NÃO substitui
  //     set_click_callback -- ambos podem ser registrados simultaneamente e ambos disparam
  //     independentemente no mesmo clique (ver o doc-comment de ClickInfo para o contrato
  //     completo, incluindo um duplo-clique disparando este callback 3 vezes no total, e a
  //     limitação conhecida do campo `button`: na prática sempre 0, já que o RmlUi nunca
  //     despacha Click/Dblclick para botões não-primários do mouse). Espaço de coordenadas:
  //     pixels físicos da janela, origem superior-esquerda, y pra baixo -- o App é dono da
  //     janela inteira, então não há offset de sub-viewport a traduzir (diferente do UiLayer).
  //     Paridade com UiLayer::set_click_info_callback (mesma assinatura).
  void set_click_info_callback(std::function<void(const ClickInfo&)> cb);

  // EN: Register a scroll callback -- reports the id of the ancestor-or-self nearest to the
  //     element whose OWN scroll offset just changed ("" if none), one call PER SOURCE OF
  //     SCROLLING glintfx does not distinguish: wheel (one call per animated smoothscroll step,
  //     not just at rest), native scrollbar thumb/track/arrow interaction, and the programmatic
  //     scroll_element_into_view()/set_element_scroll_top() calls below. glintfx does NOT play
  //     audio itself -- this is the hook a host uses to, e.g., trigger its own scroll sound.
  //     Deduped against the CURRENT offset by RmlUi itself: setting the same scroll position
  //     twice in a row does not re-fire. No numeric offset is carried by this callback (the
  //     underlying Rml::EventId::Scroll event itself carries none) -- call
  //     get_element_scroll_top(id) from inside the callback if the value is needed. Parity with
  //     UiLayer::set_scroll_callback (same signature). Null/empty callback is a safe no-op. No
  //     ordering constraint versus load().
  //
  //     WARNING (recursion, review v0.6.0 Minor #2): scroll_element_into_view()/
  //     set_element_scroll_top() below are SYNCHRONOUS and dispatch the underlying Scroll event
  //     on the SAME call stack before returning, which re-enters this callback. Calling either of
  //     them from inside the callback with a value that does NOT converge to the element's
  //     current offset (e.g. a badly-written dynamic clamp/snap that always writes something
  //     different from what it just read) recurses without bound -> stack overflow. Safe as long
  //     as the written value CONVERGES (RmlUi's own dedup -- same offset twice does not re-fire --
  //     stops the cycle, typically in 1-2 iterations for a constant target).
  // PT: Registra um callback de rolagem -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento cujo PRÓPRIO offset de rolagem acabou de mudar ("" se nenhum), uma chamada
  //     POR FONTE DE ROLAGEM que a glintfx não distingue: wheel (uma chamada por passo animado
  //     do smoothscroll, não só ao assentar), interação com a scrollbar nativa (thumb/track/
  //     setas), e as chamadas programáticas scroll_element_into_view()/set_element_scroll_top()
  //     abaixo. A glintfx NÃO toca áudio -- este é o gancho que um host usa para, ex., disparar
  //     seu próprio som de rolagem. Deduplicado contra o offset CORRENTE pelo próprio RmlUi:
  //     definir a mesma posição de rolagem duas vezes seguidas não redispara. Nenhum offset
  //     numérico é carregado por este callback (o próprio evento Rml::EventId::Scroll
  //     subjacente não carrega nenhum) -- chame get_element_scroll_top(id) de dentro do callback
  //     se o valor for necessário. Paridade com UiLayer::set_scroll_callback (mesma assinatura).
  //     Callback nulo/vazio é no-op seguro. Sem restrição de ordem vs. load().
  //
  //     AVISO (recursão, review v0.6.0 Minor #2): scroll_element_into_view()/
  //     set_element_scroll_top() abaixo são SÍNCRONOS e despacham o evento Scroll subjacente na
  //     MESMA pilha de chamada antes de retornar, o que reentra neste callback. Chamar qualquer
  //     um deles de dentro do callback com um valor que NÃO converge para o offset atual do
  //     elemento (ex.: um clamp/snap dinâmico mal escrito que sempre escreve algo diferente do
  //     que acabou de ler) recursa sem limite -> stack overflow. Seguro desde que o valor escrito
  //     CONVIRJA (o dedup do próprio RmlUi -- mesmo offset duas vezes não redispara -- para o
  //     ciclo, tipicamente em 1-2 iterações para um alvo constante).
  void set_scroll_callback(std::function<void(const char* element_id)> cb);

  // EN: Form/DOM event callbacks (L1.15-FORMEV, design doc
  //     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Shape B). Five id-
  //     resolving observer channels for RmlUi's own Change/Submit/Focus/Blur/Mouseover+Mouseout
  //     events -- glintfx does NOT play audio itself, these are the hooks a host uses to trigger
  //     its own sound/UX reaction (sonoro-relevant per the líder's explicit decision to include
  //     hover/focus). Parity with UiLayer (same signatures). See
  //     glintfx/src/bootstrap.hpp's set_change_callback/set_submit_callback/set_focus_callback/
  //     set_blur_callback/set_hover_callback doc-comments for the FULL per-event contract: id
  //     resolution (same ancestor-walk as click/scroll), the change `value` payload (RmlUi's
  //     own, per-control-kind, verbatim), the LIFETIME rule (id/value valid only for the
  //     duration of the invocation -- copy before returning if needed after), the synchronous-
  //     dispatch/reentrancy contract (MANDATORY copy-before-invoke, same AUD-TEC-3 discipline as
  //     every other callback in this family), and -- critically for set_focus_callback/
  //     set_blur_callback -- the EMPIRICALLY-VERIFIED recursion analysis (refocusing/reblurring
  //     the SAME element that already holds that state is a same-ancestor-chain no-op per
  //     RmlUi's own Context::OnFocusChange/SendEvents set-difference dedup, confirmed by reading
  //     the pinned RmlUi 6.3 source -- NOT an unbounded recursion risk; a DIFFERENT target each
  //     time IS). set_hover_callback's dedup machine (single Mouseover-only listener,
  //     `current_hover_id_` state) and its documented limitation (no window-leave exit event)
  //     are also detailed there.
  // PT: Callbacks de evento de formulário/DOM (L1.15-FORMEV, doc de design
  //     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Formato B). Cinco
  //     canais observadores que resolvem id para os próprios eventos Change/Submit/Focus/
  //     Blur/Mouseover+Mouseout do RmlUi -- a glintfx NÃO toca áudio, estes são os ganchos que
  //     um host usa para disparar sua própria reação sonora/UX (sonoro-relevante conforme
  //     decisão explícita do líder de incluir hover/focus). Paridade com UiLayer (mesmas
  //     assinaturas). Ver os doc-comments de set_change_callback/set_submit_callback/
  //     set_focus_callback/set_blur_callback/set_hover_callback em glintfx/src/bootstrap.hpp
  //     para o contrato COMPLETO por evento: resolução de id (mesma subida de ancestral de
  //     click/scroll), o payload `value` do change (o próprio do RmlUi, por tipo de controle,
  //     verbatim), a regra de LIFETIME (id/value válidos só durante a invocação -- copiar antes
  //     de retornar se precisar depois), o contrato de despacho síncrono/reentrância (cópia-
  //     antes-de-invocar OBRIGATÓRIA, mesma disciplina AUD-TEC-3 de todo outro callback desta
  //     família), e -- criticamente para set_focus_callback/set_blur_callback -- a análise de
  //     recursão VERIFICADA EMPIRICAMENTE (refocar/redesfocar o MESMO elemento que já detém
  //     aquele estado é um no-op de mesma-cadeia-de-ancestrais pelo dedup por diferença de
  //     conjunto do próprio Context::OnFocusChange/SendEvents do RmlUi, confirmado lendo o
  //     source pinado do RmlUi 6.3 -- NÃO é risco de recursão sem limite; um alvo DIFERENTE a
  //     cada vez É). A máquina de dedup de set_hover_callback (listener único só-Mouseover,
  //     estado `current_hover_id_`) e sua limitação documentada (nenhum evento de saída ao
  //     deixar a janela) também estão detalhadas lá.
  void set_change_callback(std::function<void(const char* id, const char* value)> cb);
  void set_submit_callback(std::function<void(const char* id)> cb);
  void set_focus_callback(std::function<void(const char* id)> cb);
  void set_blur_callback(std::function<void(const char* id)> cb);
  void set_hover_callback(std::function<void(const char* id, bool entered)> cb);

  // EN: Query the border-box geometry of an element by id. Coordinate space: window physical
  //     pixels, top-left origin, y-down -- App owns the whole window, so there is no sub-
  //     viewport offset to translate (unlike UiLayer, whose set_viewport(x,y,w,h,target_h)
  //     shifts this space -- see UiLayer::get_element_box). Returns ElementBox{found=false}
  //     (all fields zeroed) when the id is not found in the currently loaded document, or no
  //     document is loaded yet. Parity with UiLayer::get_element_box (same signature).
  // PT: Consulta a geometria border-box de um elemento por id. Espaço de coordenadas: pixels
  //     físicos da janela, origem superior-esquerda, y pra baixo -- o App é dono da janela
  //     inteira, então não há offset de sub-viewport a traduzir (diferente do UiLayer, cujo
  //     set_viewport(x,y,w,h,target_h) desloca esse espaço -- ver UiLayer::get_element_box).
  //     Retorna ElementBox{found=false} (todos os campos zerados) quando o id não é encontrado
  //     no documento carregado atualmente, ou nenhum documento carregado ainda. Paridade com
  //     UiLayer::get_element_box (mesma assinatura).
  ElementBox get_element_box(const char* id) const;

  // EN: Scroll an element's nearest scrolling ancestor(s) so the element becomes visible
  //     (GLINTFX-SCROLL-1, v0.4.0). Parity with UiLayer::scroll_element_into_view (same
  //     signature/semantics -- see there for the full contract: instant/synchronous, wraps
  //     Rml::Element::ScrollIntoView(bool)).
  // PT: Rola o(s) ancestral(is) rolável(eis) mais próximo(s) de um elemento para que ele fique
  //     visível (GLINTFX-SCROLL-1, v0.4.0). Paridade com UiLayer::scroll_element_into_view
  //     (mesma assinatura/semântica -- ver lá o contrato completo: instantâneo/síncrono,
  //     encapsula Rml::Element::ScrollIntoView(bool)).
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query an element's own vertical scroll offset, in RCSS pixels. Parity with
  //     UiLayer::get_element_scroll_top (same signature/semantics).
  // PT: Consulta o offset de rolagem vertical próprio de um elemento, em pixels RCSS. Paridade
  //     com UiLayer::get_element_scroll_top (mesma assinatura/semântica).
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query an element's total scrollable content height, in RCSS pixels. Parity with
  //     UiLayer::get_element_scroll_height (same signature/semantics; see there for the full
  //     scroll-metrics-trio usage formula shared with get_element_scroll_top/
  //     get_element_client_height).
  // PT: Consulta a altura total do conteúdo rolável de um elemento, em pixels RCSS. Paridade
  //     com UiLayer::get_element_scroll_height (mesma assinatura/semântica; ver lá a fórmula
  //     completa de uso do trio de métricas de rolagem, compartilhada com
  //     get_element_scroll_top/get_element_client_height).
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query an element's client (visible) height, in RCSS pixels. Parity with
  //     UiLayer::get_element_client_height (same signature/semantics).
  // PT: Consulta a altura de cliente (visível) de um elemento, em pixels RCSS. Paridade com
  //     UiLayer::get_element_client_height (mesma assinatura/semântica).
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set an element's own vertical scroll offset directly, in RCSS pixels. Parity with
  //     UiLayer::set_element_scroll_top (same signature/semantics, including the non-finite
  //     input rejection).
  // PT: Define o offset de rolagem vertical próprio de um elemento diretamente, em pixels
  //     RCSS. Paridade com UiLayer::set_element_scroll_top (mesma assinatura/semântica,
  //     incluindo a rejeição de entrada não-finita).
  bool set_element_scroll_top(const char* id, float scroll_top) const;

  // EN: Give input focus to an element, programmatically. Parity with
  //     UiLayer::set_focus (same signature/semantics, including the `focus`-property-not-
  //     `tab-index` gating contract -- see UiLayer::set_focus's doc-comment for the full
  //     writeup confirmed against the pinned RmlUi 6.3 source). (L1.17-FOCUS)
  // PT: Dá foco de entrada a um elemento, programaticamente. Paridade com
  //     UiLayer::set_focus (mesma assinatura/semântica, incluindo o contrato de controle
  //     pela propriedade `focus` -- não `tab-index` -- ver o doc-comment de UiLayer::set_focus
  //     para o relato completo confirmado contra o source pinado do RmlUi 6.3). (L1.17-FOCUS)
  bool set_focus(const char* id) const;

  // EN: Remove input focus. Parity with UiLayer::clear_focus (same signature/semantics,
  //     including the idempotent-true-when-nothing-focused contract). (L1.17-FOCUS)
  // PT: Remove o foco de entrada. Paridade com UiLayer::clear_focus (mesma
  //     assinatura/semântica, incluindo o contrato de true-idempotente-quando-nada-focado).
  //     (L1.17-FOCUS)
  bool clear_focus() const;

  // -------------------------------------------------------------------------
  // EN: DOM read/write by id (L1.16-DOMRW, consumes AUD-PUB-6(d) -- "imperative setters by
  //     id"). Parity with UiLayer (same signatures/semantics). See UiLayer's doc-comments for
  //     the full contract, including the RmlUi-signature-level detail (SetClass/SetProperty/
  //     SetInnerRML) confirmed against the pinned RmlUi 6.3 source.
  // PT: Leitura/escrita de DOM por id (L1.16-DOMRW, consome AUD-PUB-6(d) -- "setters
  //     imperativos por id"). Paridade com UiLayer (mesmas assinaturas/semântica). Ver os
  //     doc-comments do UiLayer para o contrato completo, incluindo o detalhe no nível de
  //     assinatura RmlUi (SetClass/SetProperty/SetInnerRML) confirmado contra o source pinado
  //     do RmlUi 6.3.
  // -------------------------------------------------------------------------

  // EN: Replace an element's text content. `text` is LITERAL TEXT, never markup -- `& < > "`
  //     are escaped before reaching RmlUi's SetInnerRML, so this call can NEVER be used to
  //     inject RML/tags into the document, even with untrusted `text` (chat, a player-chosen
  //     display name, etc.). Null `text` is normalised to "" (never rejected). Returns false
  //     when no document is loaded or `id` is null/empty ("")/unknown -- same AUD-TEC-5
  //     fail-high convention as get_element_box/set_focus.
  // PT: Substitui o conteúdo de texto de um elemento. `text` é TEXTO LITERAL, nunca markup --
  //     `& < > "` são escapados antes de chegar no SetInnerRML do RmlUi, então esta chamada
  //     NUNCA pode ser usada para injetar RML/tags no documento, mesmo com `text` não confiável
  //     (chat, nome de exibição escolhido pelo jogador, etc.). `text` nulo é normalizado para ""
  //     (nunca rejeitado). Retorna false quando nenhum documento estiver carregado ou `id` for
  //     nulo/vazio ("")/desconhecido -- mesma convenção fail-high AUD-TEC-5 de
  //     get_element_box/set_focus.
  bool set_text(const char* id, const char* text) const;

  // EN: Add a CSS class on an element -- wraps Rml::Element::SetClass(cls, true), which returns
  //     void (no RmlUi-level parse outcome, unlike set_property below). Returns false when no
  //     document is loaded, or `id`/`cls` is null/empty ("") -- AUD-TEC-5 fail-high, applied to
  //     BOTH parameters here (an empty class name is a structurally-invalid caller input, same
  //     as an empty id).
  // PT: Adiciona uma classe CSS num elemento -- encapsula Rml::Element::SetClass(cls, true),
  //     que retorna void (nenhum resultado de parse em nível RmlUi, diferente do set_property
  //     abaixo). Retorna false quando nenhum documento estiver carregado, ou `id`/`cls` for
  //     nulo/vazio ("") -- fail-high AUD-TEC-5, aplicado aos DOIS parâmetros aqui (um nome de
  //     classe vazio é entrada estruturalmente inválida do chamador, igual a um id vazio).
  bool add_class(const char* id, const char* cls) const;

  // EN: Remove a CSS class from an element -- wraps Rml::Element::SetClass(cls, false). Same
  //     `id`/`cls` null/empty guard as add_class. Removing a class the element never had is a
  //     safe no-op inside RmlUi itself, so this still returns true in that case.
  // PT: Remove uma classe CSS de um elemento -- encapsula Rml::Element::SetClass(cls, false).
  //     Mesmo guard de `id`/`cls` nulo/vazio do add_class. Remover uma classe que o elemento
  //     nunca teve é um no-op seguro dentro do próprio RmlUi, então ainda retorna true nesse
  //     caso.
  bool remove_class(const char* id, const char* cls) const;

  // EN: Set a single inline RCSS property by name -- wraps Rml::Element::SetProperty(name,
  //     value), which DOES return a real bool (RmlUi's own parser rejects an unparseable
  //     `value` for the named property and this call propagates that rejection unchanged --
  //     no separate glintfx-level validation on top). `id`/`prop` null or empty ("") is
  //     rejected (false) before touching RmlUi -- AUD-TEC-5 fail-high. `value` null is
  //     normalised to "" and forwarded to RmlUi's own parser (which is the sole authority on
  //     whether an empty value is acceptable for the named property).
  // PT: Define uma única propriedade RCSS inline por nome -- encapsula
  //     Rml::Element::SetProperty(name, value), que DE FATO retorna um bool real (o próprio
  //     parser do RmlUi rejeita um `value` que não parseia para a propriedade nomeada e esta
  //     chamada propaga essa rejeição sem mudança -- nenhuma validação separada em nível
  //     glintfx por cima). `id`/`prop` nulo ou vazio ("") é rejeitado (false) antes de tocar o
  //     RmlUi -- fail-high AUD-TEC-5. `value` nulo é normalizado para "" e encaminhado ao
  //     próprio parser do RmlUi (que é a única autoridade sobre se um valor vazio é aceitável
  //     para a propriedade nomeada).
  bool set_property(const char* id, const char* prop, const char* value) const;

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1) — parity with UiLayer. Call order: create_data_model
  //     -> bind_* -> load() -> set_*(). Engine enforces the ordering constraint
  //     (bind_* after load() returns false). No engine-specific types in this header.
  // PT: API de data-model (T1) — paridade com UiLayer. Ordem de chamada:
  //     create_data_model -> bind_* -> load() -> set_*(). Engine enforça a
  //     restrição de ordem (bind_* após load() retorna false). API usa tipos C simples.
  // -------------------------------------------------------------------------

  // EN: Create a data model. Call before bind_* and before load(). LIMIT: at most ONE data
  //     model per App instance -- a second create_data_model() call (any name, including a
  //     different one) returns false and is a silent no-op; the first model stays the only one
  //     bound (DataBinder's `created` flag, checked-before-mutate). Not currently surfaced to
  //     the caller other than the false return -- if multiple independent models are needed,
  //     bind all variables under the ONE model created here. AUD-TEC-7 (2026-07-08).
  // PT: Cria um data model. Chamar antes de bind_* e antes de load(). LIMITE: no máximo UM
  //     data model por instância de App -- uma segunda chamada a create_data_model() (qualquer
  //     nome, mesmo diferente) retorna false e é um no-op silencioso; o primeiro modelo
  //     permanece o único ligado (flag `created` do DataBinder, check-before-mutate). Não é
  //     sinalizado ao chamador além do retorno false -- se múltiplos modelos independentes
  //     forem necessários, ligue todas as variáveis sob o ÚNICO modelo criado aqui.
  //     AUD-TEC-7 (2026-07-08).
  bool create_data_model(const char* name);

  // EN: Bind a numeric (double) cell with an optional initial value.
  // PT: Liga uma célula numérica (double) com valor inicial opcional.
  bool bind_number(const char* key, double initial = 0.0);

  // EN: Bind a string cell.
  // PT: Liga uma célula de string.
  bool bind_string(const char* key, const char* initial = "");

  // EN: Bind a boolean cell.
  // PT: Liga uma célula booleana.
  bool bind_bool  (const char* key, bool initial = false);

  // EN: Bind a string-list cell (for data-for iteration in RML).
  // PT: Liga uma célula de lista de strings (para iteração data-for no RML).
  bool bind_list  (const char* key);

  // EN: Update a numeric cell after load. No-op when key is unknown.
  //     The change is reflected on the next update() call.
  // PT: Atualiza uma célula numérica após load. No-op quando a chave é desconhecida.
  //     A mudança é refletida na próxima chamada a update().
  void set_number(const char* key, double value);

  // EN: Update a string cell after load.
  // PT: Atualiza uma célula de string após load.
  void set_string(const char* key, const char* value);

  // EN: Update a boolean cell after load.
  // PT: Atualiza uma célula booleana após load.
  void set_bool  (const char* key, bool value);

  // EN: Replace the entire string list and dirty the variable.
  //     items[0..count-1] are copied; caller retains ownership.
  // PT: Substitui a lista de strings inteira e marca a variável suja.
  //     items[0..count-1] são copiados; o chamador retém a propriedade.
  void set_list  (const char* key, const char* const* items, std::size_t count);

  // EN: Read-back the CURRENT value of a bound cell (L1.16-DOMRW, consumes AUD-PUB-6(e) -- the
  //     data-model was write-only before this: a host could push values in via set_*, but had
  //     no way to read back a value the UI itself, or the user through a bound control (e.g. an
  //     `<input data-value>`), wrote into the model. Reflects any such RmlUi-driven
  //     bidirectional write, not just the host's own last set_* call -- see
  //     glintfx/src/data_binder.hpp's get_number doc-comment for the pointer-identity reasoning
  //     confirmed against the pinned RmlUi source. Returns false (out left untouched) when the
  //     key was never bound; no ordering constraint versus load() beyond that (a bound key is
  //     readable before or after load()). Parity with UiLayer::get_number/get_string/get_bool
  //     (same signatures).
  // PT: Lê de volta o valor CORRENTE de uma célula ligada (L1.16-DOMRW, consome AUD-PUB-6(e) --
  //     o data-model era write-only antes disto: um host podia empurrar valores via set_*, mas
  //     não tinha como ler de volta um valor que a própria UI, ou o usuário através de um
  //     controle ligado (ex.: um `<input data-value>`), escreveu no modelo. Reflete qualquer
  //     escrita bidirecional dirigida pelo RmlUi assim, não só a última chamada set_* do
  //     próprio host -- ver o doc-comment de get_number em glintfx/src/data_binder.hpp para o
  //     raciocínio de identidade-de-ponteiro confirmado contra o source pinado do RmlUi. Retorna
  //     false (out intocado) quando a chave nunca foi ligada; sem restrição de ordem vs. load()
  //     além disso (uma chave ligada é legível antes ou depois de load()). Paridade com
  //     UiLayer::get_number/get_string/get_bool (mesmas assinaturas).
  bool get_number(const char* key, double& out) const;
  bool get_string(const char* key, std::string& out) const;
  bool get_bool  (const char* key, bool& out) const;

  // EN: Render one frame and save the result to a PPM file before swapping buffers.
  //     This captures the composited image from the window framebuffer (FBO 0) immediately
  //     after EndFrame composites the render layer, ensuring correct pixel data even on
  //     platforms where the back buffer content is undefined after glfwSwapBuffers.
  //     Returns true on success.
  // PT: Renderiza um frame e salva o resultado em arquivo PPM antes de trocar buffers.
  //     Captura a imagem composta do framebuffer da janela (FBO 0) imediatamente após
  //     o EndFrame compositar o render layer, garantindo dados corretos mesmo em plataformas
  //     onde o back buffer fica indefinido após glfwSwapBuffers.
  //     Retorna true em caso de sucesso.
  bool snapshot(const char* ppm_path);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
