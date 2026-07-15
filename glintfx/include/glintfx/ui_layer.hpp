// SPDX-License-Identifier: MPL-2.0
// EN: Embed/guest facade — attaches the UI+effects engine to a GL context the HOST owns.
//     Does not create a window; render() is compose-only (no clear, no swap).
// PT: Fachada embed/guest — anexa o motor de UI+efeitos a um contexto GL do HOST.
//     Não cria janela; render() é compose-only (sem clear, sem swap).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <memory>
#include <cstddef>
#include <functional>
#include <string>  // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).
#include <glintfx/ui_event.hpp>
#include <glintfx/element_box.hpp>
#include <glintfx/click_info.hpp>
#include <glintfx/font_engine.hpp>

namespace glintfx {

// EN: Configuration for UiLayer — specifies logical viewport dimensions and GL loader policy.
//     Declared at namespace scope so it is a complete type when used as a default argument
//     in the UiLayer constructor (inner-class default-args hit a C++ language restriction).
// PT: Configuração para UiLayer — especifica dimensões lógicas do viewport e política do GL loader.
//     Declarada em namespace scope para ser tipo completo ao ser usada como argumento padrão
//     no construtor do UiLayer (default-args de inner-class atingem uma restrição da linguagem C++).
struct UiLayerConfig {
  int logical_width  = 1280;
  int logical_height = 720;
  bool load_gl       = true;
  // EN: Initial density-independent pixel ratio (dp_ratio).
  //     1 dp = dp_ratio physical pixels. Set > 1.0 when the host renders at a higher
  //     physical resolution than the logical RCSS canvas (e.g. dp_ratio=2.0 means 1dp=2px).
  //     Can be updated at runtime via set_dp_ratio(). Default 1.0 = no scaling.
  // PT: Density-independent pixel ratio inicial (dp_ratio).
  //     1 dp = dp_ratio pixels físicos. Defina > 1.0 quando o host renderiza numa resolução
  //     física maior que o canvas lógico RCSS (ex.: dp_ratio=2.0 significa 1dp=2px).
  //     Pode ser atualizado em runtime via set_dp_ratio(). Padrão 1.0 = sem escala.
  float dp_ratio = 1.0f;
  // EN: Which font engine to install (L1.20-FONTFLIP Phase 2). Fixed at construction --
  //     UiLayer reads this exactly once, during the ctor's Bootstrap::init() call, before
  //     Rml::Initialise(); there is no runtime setter (see FontEngine's own doc-comment for the
  //     full contract, including the GLINTFX_OWN_FONT_ENGINE=OFF fallback-to-FreeType path).
  //     Default: FontEngine::Own -- the "soft flip" (L1.20-FONTFLIP Phase 1) makes glintfx's own
  //     clean-room font engine the default; set to FontEngine::FreeType for a one-line,
  //     no-rebuild rollback to RmlUi's built-in engine. Parity with AppConfig::font_engine (same
  //     field name/semantics).
  // PT: Qual motor de fonte instalar (L1.20-FONTFLIP Fase 2). Fixado na construção -- o UiLayer
  //     lê isto exatamente uma vez, durante a chamada a Bootstrap::init() do ctor, antes do
  //     Rml::Initialise(); não há setter de runtime (ver o doc-comment do próprio FontEngine
  //     para o contrato completo, incluindo o caminho de fallback-para-FreeType quando
  //     GLINTFX_OWN_FONT_ENGINE=OFF). Default: FontEngine::Own -- o "flip suave"
  //     (L1.20-FONTFLIP Fase 1) torna o motor de fonte próprio clean-room do glintfx o padrão;
  //     defina FontEngine::FreeType para um rollback de uma linha, sem rebuild, ao motor
  //     embutido do RmlUi. Paridade com AppConfig::font_engine (mesmo nome de campo/semântica).
  FontEngine font_engine = FontEngine::Own;
};

// EN: MOVED-FROM STATE (L1.10-APIDOC): after `UiLayer b = std::move(a);` (or
//     `b = std::move(a);`), `a` is left in a moved-from state. Calling ANY method on `a`
//     other than ok() or the destructor is undefined behaviour — same convention as
//     std::unique_ptr. ok() on a moved-from UiLayer returns false (impl_ is null), which is
//     the only state query that is safe to make; destruction of a moved-from UiLayer is
//     always safe (no-op, impl_ is null).
// PT: ESTADO MOVIDO-DE (L1.10-APIDOC): após `UiLayer b = std::move(a);` (ou
//     `b = std::move(a);`), `a` fica em estado movido-de. Chamar QUALQUER método em `a`
//     além de ok() ou o destrutor é comportamento indefinido — mesma convenção do
//     std::unique_ptr. ok() num UiLayer movido-de retorna false (impl_ é nulo), sendo a
//     única consulta de estado segura; destruir um UiLayer movido-de é sempre seguro
//     (no-op, impl_ é nulo).
class UiLayer {
public:
  // EN: Inner alias so callers can write UiLayer::Config{...} — matches the spec interface.
  // PT: Alias interno para que chamadores possam escrever UiLayer::Config{...} — segue a spec.
  using Config = UiLayerConfig;

  // EN: Requires a CURRENT GL context owned by the host.
  //     GL function pointers are loaded (idempotent per-process) when cfg.load_gl is true.
  // PT: Exige contexto GL CORRENTE do host.
  //     Ponteiros de função GL são carregados (idempotente por processo) quando cfg.load_gl for true.
  explicit UiLayer(Config cfg = Config{});
  ~UiLayer();

  // EN: Move leaves the source in a moved-from state — see the class-level comment above
  //     for the exact contract (only ok() and ~UiLayer() remain valid on the source afterwards).
  // PT: O move deixa a origem em estado movido-de — ver o comentário de nível de classe
  //     acima para o contrato exato (só ok() e ~UiLayer() permanecem válidos na origem depois).
  UiLayer(UiLayer&&) noexcept;
  UiLayer& operator=(UiLayer&&) noexcept;
  UiLayer(const UiLayer&)            = delete;
  UiLayer& operator=(const UiLayer&) = delete;

  // EN: True when attach + engine init succeeded.
  // PT: True quando attach + init do motor foram bem-sucedidos.
  bool ok() const noexcept;

  // EN: Load a document from rml_path and call Show(). Requires ok().
  //     Returns true on success.
  // PT: Carrega documento de rml_path e chama Show(). Requer ok().
  //     Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Notify a viewport resize (2-arg, unchanged): real target pixels, no sub-region
  //     offset -- equivalent to set_viewport(0, 0, w, h, /*target_h=*/h).
  // PT: Notifica redimensionamento de viewport (2 args, inalterado): pixels reais do alvo,
  //     sem offset de sub-região -- equivalente a set_viewport(0, 0, w, h, /*target_h=*/h).
  void set_viewport(int w, int h);

  // EN: Notify a viewport resize AND placement within a larger host render target (letterbox,
  //     F3, v0.2.5). (x, y): top-down, window-space pixel offset of the sub-viewport's
  //     top-left corner -- the SAME convention as UiEvent's mouse coordinates (see
  //     process_event below) and as get_element_box()'s return value. (w, h): the
  //     sub-viewport's own logical/content dimensions (RmlUi context size), unaffected by
  //     the offset. target_h: the FULL height of the host's render target (its window), in
  //     physical pixels -- used ONLY to convert (x, y) into the OpenGL-native bottom-up
  //     offset the final composite needs (glViewport is bottom-left-origin; see the
  //     ADR-0008 "F3 implementation update" section for the exact formula and rationale).
  //     No target_w parameter: the X axis needs no conversion between the two conventions.
  //     Precondition: target_h >= y + h (not enforced -- an inconsistent value composites at
  //     an unexpected but harmless on-screen position, never a crash).
  // PT: Notifica redimensionamento de viewport E posicionamento dentro de um render target
  //     maior do host (letterbox, F3, v0.2.5). (x, y): offset em pixels, espaço-janela
  //     topo-esquerda, do canto superior esquerdo da sub-viewport -- a MESMA convenção das
  //     coordenadas de mouse do UiEvent (ver process_event abaixo) e do retorno de
  //     get_element_box(). (w, h): as dimensões lógicas/de conteúdo da própria sub-viewport
  //     (tamanho do contexto RmlUi), não afetadas pelo offset. target_h: a altura TOTAL do
  //     render target do host (a janela dele), em pixels físicos -- usada SÓ pra converter
  //     (x, y) no offset nativo bottom-up que o OpenGL exige na composição final (glViewport
  //     tem origem inferior-esquerda; ver a seção "F3 implementation update" do ADR-0008 pra
  //     fórmula exata e justificativa). Sem parâmetro target_w: o eixo X não precisa de
  //     conversão entre as duas convenções. Pré-condição: target_h >= y + h (não enforçada --
  //     um valor inconsistente compõe numa posição de tela inesperada mas inofensiva, nunca
  //     crash).
  void set_viewport(int x, int y, int w, int h, int target_h);

  // EN: Update the density-independent pixel ratio at runtime.
  //     1 dp (in RCSS) = ratio physical pixels. Triggers a layout re-pass when changed.
  //     Example: host renders 1920×1080 with RCSS authored at 960×540 → set_dp_ratio(2.0f).
  //     Pairs with set_viewport(real_w, real_h) which stays in physical pixels.
  // PT: Atualiza o density-independent pixel ratio em runtime.
  //     1 dp (no RCSS) = ratio pixels físicos. Dispara re-layout quando alterado.
  //     Exemplo: host renderiza 1920×1080 com RCSS no canvas lógico 960×540 → set_dp_ratio(2.0f).
  //     Usa com set_viewport(real_w, real_h) que permanece em pixels físicos.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL for asset resolution (fonts, images, RCSS).
  //     When set, relative paths passed to Open() are resolved as base_url/path.
  //     Absolute paths are unaffected. Call before load() for the URL to take effect.
  //     Pass nullptr or "" to clear. Safe to call after ok() is true.
  // PT: Sobrepõe o base URL para resolução de assets (fontes, imagens, RCSS).
  //     Quando definido, caminhos relativos passados a Open() são resolvidos como base_url/path.
  //     Caminhos absolutos não são afetados. Chame antes de load() para o URL ter efeito.
  //     Passe nullptr ou "" para limpar. Seguro chamar após ok() ser true.
  void set_asset_base_url(const char* url);

  // EN: Advance the UI context by one step (call once per frame before render()).
  // PT: Avança o contexto de UI um passo (chamar uma vez por frame antes de render()).
  void update();

  // EN: Compose-only render -- overlays the UI onto the window backbuffer (FBO 0) via
  //     premultiplied-alpha blend. No glClear. No buffer swap. GL state is automatically
  //     saved and restored on return (viewport, blend func+equation, scissor, program, VAO,
  //     texture, draw-FBO binding, colour mask).
  //
  //     F1 contract limitation: the internal graphics backend unconditionally targets the
  //     window backbuffer (FBO 0) during EndFrame; a custom host FBO bound before this call
  //     is NOT used as the composition target (see ADR-0008). The host must render its scene
  //     into the window backbuffer and call this method after drawing its scene, before swap.
  //     The viewport ORIGIN within FBO 0 is configurable since v0.2.5 via the 5-arg
  //     set_viewport(x, y, w, h, target_h) overload (letterbox) -- see its doc comment and
  //     the ADR-0008 "F3 implementation update" section. When letterboxing, the host must
  //     clear/redraw its scene across the FULL window (not just the sub-region) before
  //     calling render() -- render() itself never clears.
  //
  // PT: Render compose-only -- sobrepõe a UI ao backbuffer da janela (FBO 0) via blend
  //     premultiplied-alpha. Sem glClear. Sem swap. O estado GL é salvo e restaurado
  //     automaticamente ao retornar (viewport, blend func+equation, scissor, programa, VAO,
  //     textura, draw-FBO binding, colour mask).
  //
  //     Limitação de contrato F1: o backend gráfico interno mira incondicionalmente o backbuffer
  //     da janela (FBO 0) durante EndFrame; um FBO customizado do host ligado antes desta chamada
  //     NÃO é usado como alvo de composição (ver ADR-0008). O host deve renderizar sua cena no
  //     backbuffer da janela e chamar este método após desenhar a cena, antes do swap.
  //     A ORIGEM do viewport dentro do FBO 0 é configurável desde a v0.2.5 via a sobrecarga de
  //     5 args set_viewport(x, y, w, h, target_h) (letterbox) -- ver o doc-comment dela e a
  //     seção "F3 implementation update" do ADR-0008. Ao letterboxar, o host deve limpar/
  //     redesenhar sua cena na janela INTEIRA (não só a sub-região) antes de chamar render() --
  //     o render() em si nunca limpa.
  void render();

  // EN: Inject a host input event — translated to engine input calls internally (T4).
  // PT: Injeta evento de entrada do host — traduzido para chamadas de input do motor internamente (T4).
  void process_event(const UiEvent& ev);

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none reached before the document root).
  //     No ordering constraint versus load(): safe to call before or after it -- the listener
  //     always reads the CURRENT callback at click time, not the value at attach time. Parity
  //     with App::set_click_callback (same signature).
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum for encontrado até a raiz do
  //     documento). Sem restrição de ordem vs. load(): seguro chamar antes ou depois -- o
  //     listener sempre lê o callback CORRENTE no momento do clique, não o valor no attach.
  //     Paridade com App::set_click_callback (mesma assinatura).
  void set_click_callback(std::function<void(const char* element_id)> cb);

  // EN: Register a RICHER click callback -- id + button + coordinates + double-click, a
  //     second/parallel channel alongside set_click_callback (AUD-PUB-4, v0.5.0). Does NOT
  //     replace set_click_callback -- both can be registered simultaneously and both fire
  //     independently on the same click (see ClickInfo's doc-comment for the full contract,
  //     including a double-click firing this callback 3 times total, and the `button` field's
  //     known limitation: effectively always 0, since RmlUi never dispatches Click/Dblclick for
  //     non-primary mouse buttons). Coordinate space: window/render-target physical pixels,
  //     top-left origin, y-down -- SAME convention as get_element_box (the sub-viewport offset
  //     from set_viewport(x,y,w,h,target_h) is added here, mirroring get_element_box's
  //     translation). No ordering constraint versus load(). Parity with
  //     App::set_click_info_callback (same signature).
  // PT: Registra um callback de clique MAIS RICO -- id + botão + coordenadas + duplo-clique,
  //     um segundo canal/paralelo ao set_click_callback (AUD-PUB-4, v0.5.0). NÃO substitui
  //     set_click_callback -- ambos podem ser registrados simultaneamente e ambos disparam
  //     independentemente no mesmo clique (ver o doc-comment de ClickInfo para o contrato
  //     completo, incluindo um duplo-clique disparando este callback 3 vezes no total, e a
  //     limitação conhecida do campo `button`: na prática sempre 0, já que o RmlUi nunca
  //     despacha Click/Dblclick para botões não-primários do mouse). Espaço de coordenadas:
  //     pixels físicos do render-target/janela, origem superior-esquerda, y pra baixo -- MESMA
  //     convenção de get_element_box (o offset de sub-viewport de
  //     set_viewport(x,y,w,h,target_h) é somado aqui, espelhando a tradução de
  //     get_element_box). Sem restrição de ordem vs. load(). Paridade com
  //     App::set_click_info_callback (mesma assinatura).
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
  //     get_element_scroll_top(id) from inside the callback if the value is needed; no
  //     coordinate-space translation applies to the id string (unlike set_click_info_callback's
  //     x/y, there is no numeric payload here to translate). Parity with App::set_scroll_callback
  //     (same signature). Null/empty callback is a safe no-op. No ordering constraint versus
  //     load().
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
  //     se o valor for necessário; nenhuma tradução de espaço de coordenada se aplica à string de
  //     id (diferente do x/y do set_click_info_callback, não há payload numérico aqui a
  //     traduzir). Paridade com App::set_scroll_callback (mesma assinatura). Callback nulo/vazio
  //     é no-op seguro. Sem restrição de ordem vs. load().
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
  //     hover/focus). No coordinate-space translation applies to any of these (unlike
  //     get_element_box/set_click_info_callback's x/y) -- they carry only id (+ value for
  //     change), not geometry, so the sub-viewport offset from set_viewport(x,y,w,h,target_h) is
  //     irrelevant here (same reasoning as the DOM read/write methods below). Parity with
  //     App::set_change_callback/set_submit_callback/set_focus_callback/set_blur_callback/
  //     set_hover_callback (same signatures). See glintfx/src/bootstrap.hpp's doc-comments on
  //     the same-named methods for the FULL per-event contract: id resolution (same ancestor-
  //     walk as click/scroll), the change `value` payload (RmlUi's own, per-control-kind,
  //     verbatim), the LIFETIME rule (id/value valid only for the duration of the invocation --
  //     copy before returning if needed after), the synchronous-dispatch/reentrancy contract
  //     (MANDATORY copy-before-invoke, same AUD-TEC-3 discipline as every other callback in this
  //     family), and -- critically for set_focus_callback/set_blur_callback -- the
  //     EMPIRICALLY-VERIFIED recursion analysis (refocusing/reblurring the SAME element that
  //     already holds that state is a same-ancestor-chain no-op per RmlUi's own
  //     Context::OnFocusChange/SendEvents set-difference dedup, confirmed by reading the pinned
  //     RmlUi 6.3 source -- NOT an unbounded recursion risk; a DIFFERENT target each time IS).
  //     set_hover_callback's dedup machine (single Mouseover-only listener, `current_hover_id_`
  //     state) and its documented limitation (no window-leave exit event) are also detailed
  //     there.
  // PT: Callbacks de evento de formulário/DOM (L1.15-FORMEV, doc de design
  //     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Formato B). Cinco
  //     canais observadores que resolvem id para os próprios eventos Change/Submit/Focus/
  //     Blur/Mouseover+Mouseout do RmlUi -- a glintfx NÃO toca áudio, estes são os ganchos que
  //     um host usa para disparar sua própria reação sonora/UX (sonoro-relevante conforme
  //     decisão explícita do líder de incluir hover/focus). Nenhuma tradução de espaço de
  //     coordenada se aplica a nenhum destes (diferente do x/y de
  //     get_element_box/set_click_info_callback) -- carregam só id (+ value pro change), não
  //     geometria, então o offset de sub-viewport de set_viewport(x,y,w,h,target_h) é
  //     irrelevante aqui (mesma racional dos métodos de leitura/escrita de DOM abaixo). Paridade
  //     com App::set_change_callback/set_submit_callback/set_focus_callback/set_blur_callback/
  //     set_hover_callback (mesmas assinaturas). Ver os doc-comments dos métodos de mesmo nome
  //     em glintfx/src/bootstrap.hpp para o contrato COMPLETO por evento: resolução de id (mesma
  //     subida de ancestral de click/scroll), o payload `value` do change (o próprio do RmlUi,
  //     por tipo de controle, verbatim), a regra de LIFETIME (id/value válidos só durante a
  //     invocação -- copiar antes de retornar se precisar depois), o contrato de despacho
  //     síncrono/reentrância (cópia-antes-de-invocar OBRIGATÓRIA, mesma disciplina AUD-TEC-3 de
  //     todo outro callback desta família), e -- criticamente para set_focus_callback/
  //     set_blur_callback -- a análise de recursão VERIFICADA EMPIRICAMENTE (refocar/redesfocar
  //     o MESMO elemento que já detém aquele estado é um no-op de mesma-cadeia-de-ancestrais
  //     pelo dedup por diferença de conjunto do próprio Context::OnFocusChange/SendEvents do
  //     RmlUi, confirmado lendo o source pinado do RmlUi 6.3 -- NÃO é risco de recursão sem
  //     limite; um alvo DIFERENTE a cada vez É). A máquina de dedup de set_hover_callback
  //     (listener único só-Mouseover, estado `current_hover_id_`) e sua limitação documentada
  //     (nenhum evento de saída ao deixar a janela) também estão detalhadas lá.
  void set_change_callback(std::function<void(const char* id, const char* value)> cb);
  void set_submit_callback(std::function<void(const char* id)> cb);
  void set_focus_callback(std::function<void(const char* id)> cb);
  void set_blur_callback(std::function<void(const char* id)> cb);
  void set_hover_callback(std::function<void(const char* id, bool entered)> cb);

  // EN: Query the border-box geometry of an element by id. Coordinate space: window/render-
  //     target physical pixels, top-left origin, y-down -- the SAME space as UiEvent's mouse
  //     coordinates (see ElementBox doc-comment and docs/embed-integration.md section 10).
  //     Border-box (includes border, excludes margin). Returns ElementBox{found=false} (all
  //     fields zeroed) when the id is not found in the currently loaded document, or no
  //     document is loaded yet. Parity with App::get_element_box (same signature).
  // PT: Consulta a geometria border-box de um elemento por id. Espaço de coordenadas: pixels
  //     físicos do render-target/janela, origem superior-esquerda, y pra baixo -- o MESMO
  //     espaço das coordenadas de mouse do UiEvent (ver doc-comment de ElementBox e
  //     docs/embed-integration.md seção 10). Border-box (inclui borda, exclui margem). Retorna
  //     ElementBox{found=false} (todos os campos zerados) quando o id não é encontrado no
  //     documento carregado atualmente, ou nenhum documento carregado ainda. Paridade com
  //     App::get_element_box (mesma assinatura).
  ElementBox get_element_box(const char* id) const;

  // EN: Scroll an element's nearest scrolling ancestor(s) (e.g. an overflow-y:auto container)
  //     so the element becomes visible (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven by
  //     GusWorld: keyboard/gamepad selection moving inside a 30-item menu list had no way to
  //     "follow" the selection out of the visible area). Wraps Rml::Element::ScrollIntoView(bool):
  //     align_with_top true aligns the element to the top of the scrollable view, false to the
  //     bottom. Instant (synchronous) -- the resulting scroll offset is readable via
  //     get_element_scroll_top() immediately after this call returns, no update() needed in
  //     between. Returns false (no-op) when the id is not found in the currently loaded
  //     document, or no document is loaded yet -- same guard shape as get_element_box.
  //     Typical use, gamepad/keyboard menu navigation (the GusWorld scenario):
  //       // On Key::Down moving selection to the next item:
  //       ui.set_string("selected_id", next_item_id);        // app-side selection state
  //       ui.scroll_element_into_view(next_item_id);          // follow it into view
  // PT: Rola o(s) ancestral(is) rolável(eis) mais próximo(s) de um elemento (ex.: um container
  //     overflow-y:auto) para que ele fique visível (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven
  //     pelo GusWorld: a seleção por teclado/gamepad se movendo dentro de uma lista de menu de
  //     30 itens não tinha como "seguir" a seleção para fora da área visível). Encapsula
  //     Rml::Element::ScrollIntoView(bool): align_with_top true alinha o elemento ao topo da
  //     área rolável, false ao fundo. Instantâneo (síncrono) -- o offset de rolagem resultante
  //     é lido via get_element_scroll_top() imediatamente após esta chamada retornar, sem
  //     precisar de update() no meio. Retorna false (no-op) quando o id não é encontrado no
  //     documento carregado atualmente, ou nenhum documento carregado ainda -- mesmo formato de
  //     guard de get_element_box.
  //     Uso típico, navegação de menu por gamepad/teclado (o cenário do GusWorld):
  //       // No Key::Down movendo a seleção para o próximo item:
  //       ui.set_string("selected_id", next_item_id);        // estado de seleção do app
  //       ui.scroll_element_into_view(next_item_id);          // segue ele até a área visível
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query an element's own vertical scroll offset, in RCSS pixels (e.g. how far an
  //     overflow-y:auto container has been scrolled down). Returns false (out_scroll_top left
  //     untouched) when the id is not found or no document is loaded yet -- same guard shape as
  //     get_element_box. Chosen as bool + out-param (not an ElementBox-style aggregate struct):
  //     a single scalar does not benefit from a named 4-field struct the way border-box
  //     geometry does; this mirrors the internal Bootstrap/Engine signature 1:1 with no
  //     translation at this boundary.
  // PT: Consulta o offset de rolagem vertical próprio de um elemento, em pixels RCSS (ex.:
  //     quanto um container overflow-y:auto foi rolado para baixo). Retorna false
  //     (out_scroll_top intocado) quando o id não é encontrado ou nenhum documento carregado
  //     ainda -- mesmo formato de guard de get_element_box. Escolhido como bool + out-param (não
  //     um struct agregado no estilo ElementBox): um único escalar não se beneficia de um struct
  //     nomeado de 4 campos como a geometria border-box se beneficia; isso espelha 1:1 a
  //     assinatura interna de Bootstrap/Engine, sem tradução nesta fronteira.
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query the total scrollable content height of an element, in RCSS pixels (its own
  //     padding included, margin excluded -- same box-model slice RmlUi documents for
  //     GetScrollHeight()). Completes the scroll-metrics trio with get_element_scroll_top() and
  //     get_element_client_height() below -- exactly the three values RmlUi itself groups
  //     together (see Element.h) and everything a host needs to build a CUSTOM scrollbar (the
  //     builtin one already renders when the RCSS declares overflow-y: scroll/auto; this trio
  //     is for hosts that want to draw their own indicator instead, e.g. matching a game's HUD
  //     style):
  //       float scroll_top = 0.f, scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_top("list", scroll_top);
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       float scroll_fraction = (scroll_h > client_h)
  //           ? scroll_top / (scroll_h - client_h) : 0.f;      // 0..1, guard divide-by-zero
  //       float thumb_size_fraction = client_h / scroll_h;      // 0..1
  //     Returns false (out_scroll_height left untouched) when the id is not found or no
  //     document is loaded yet -- same guard shape as get_element_box.
  // PT: Consulta a altura total do conteúdo rolável de um elemento, em pixels RCSS (padding
  //     próprio incluso, margem excluída -- a mesma fatia do box-model que o RmlUi documenta
  //     para GetScrollHeight()). Completa o trio de métricas de rolagem com
  //     get_element_scroll_top() e get_element_client_height() abaixo -- exatamente os três
  //     valores que o próprio RmlUi agrupa (ver Element.h) e tudo que um host precisa para
  //     construir uma scrollbar CUSTOMIZADA (a embutida já renderiza quando o RCSS declara
  //     overflow-y: scroll/auto; este trio é para hosts que querem desenhar seu próprio
  //     indicador, ex.: casando com o estilo de HUD de um jogo):
  //       float scroll_top = 0.f, scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_top("list", scroll_top);
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       float fracao_rolada = (scroll_h > client_h)
  //           ? scroll_top / (scroll_h - client_h) : 0.f;      // 0..1, guardar divisão por zero
  //       float fracao_tamanho_thumb = client_h / scroll_h;     // 0..1
  //     Retorna false (out_scroll_height intocado) quando o id não é encontrado ou nenhum
  //     documento carregado ainda -- mesmo formato de guard de get_element_box.
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query the client (visible) height of an element, in RCSS pixels -- the third member of
  //     the scroll-metrics trio (see get_element_scroll_height()'s doc-comment above for the
  //     full usage formula). Returns false (out_client_height left untouched) when the id is
  //     not found or no document is loaded yet -- same guard shape as get_element_box.
  // PT: Consulta a altura de cliente (visível) de um elemento, em pixels RCSS -- o terceiro
  //     membro do trio de métricas de rolagem (ver o doc-comment de get_element_scroll_height()
  //     acima para a fórmula de uso completa). Retorna false (out_client_height intocado)
  //     quando o id não é encontrado ou nenhum documento carregado ainda -- mesmo formato de
  //     guard de get_element_box.
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set an element's own vertical scroll offset directly, in RCSS pixels. RmlUi clamps the
  //     value internally to [0, scroll_height - client_height] -- an out-of-range value is
  //     saturated, not rejected. Applied synchronously (not animated): a subsequent
  //     get_element_scroll_top() call reads the new (clamped) value back immediately.
  //     Input hardening: rejects non-finite scroll_top (NaN/inf) with false and no effect --
  //     same fail-high spirit as set_dp_ratio()'s NaN/inf guard (v0.3.0 hardening audit).
  //     Returns false when the id is not found, no document is loaded yet, or scroll_top is
  //     not finite.
  //     Typical use, a custom scrollbar thumb the host draws and drags itself:
  //       float scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       ui.set_element_scroll_top("list", drag_fraction * (scroll_h - client_h));
  // PT: Define o offset de rolagem vertical próprio de um elemento diretamente, em pixels
  //     RCSS. O RmlUi satura o valor internamente em [0, scroll_height - client_height] -- um
  //     valor fora do intervalo é saturado, não rejeitado. Aplicado sincronamente (não
  //     animado): uma chamada subsequente a get_element_scroll_top() lê o novo valor (saturado)
  //     de volta imediatamente.
  //     Hardening de entrada: rejeita scroll_top não-finito (NaN/inf) com false e sem efeito --
  //     mesmo espírito fail-high do guard de NaN/inf de set_dp_ratio() (auditoria de hardening
  //     v0.3.0). Retorna false quando o id não é encontrado, nenhum documento carregado ainda,
  //     ou scroll_top não é finito.
  //     Uso típico, um thumb de scrollbar customizado que o host desenha e arrasta:
  //       float scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       ui.set_element_scroll_top("list", drag_fraction * (scroll_h - client_h));
  bool set_element_scroll_top(const char* id, float scroll_top) const;

  // EN: Give input focus to an element, programmatically (L1.17-FOCUS -- consumes the GAP-4
  //     seed documented in docs/embed-integration.md section 5: hosts whose MODEL owns
  //     selection, e.g. a game menu driven by data-binding rather than RmlUi's own Tab/arrow
  //     navigation). Wraps Rml::Element::Focus(false).
  //     IMPORTANT: gated by the element's RCSS `focus` property (keyword `auto`|`none`), whose
  //     RmlUi-registered default is `auto` -- i.e. by default EVERY element (including a plain
  //     `<div>`) accepts this call, UNLESS it is explicitly authored `focus: none;`. This is
  //     NOT the same property as `tab-index` (Tab-key traversal ORDER only, default `none`,
  //     has zero effect on this method) -- there is no RmlUi/glintfx equivalent of HTML's
  //     implicit "only tabindex-bearing elements are focusable" rule. See
  //     glintfx/src/bootstrap.hpp's set_focus doc-comment for the full contract confirmed
  //     against the pinned RmlUi 6.3 source.
  //     Returns false when no document is loaded, id is null/empty ("") -- same AUD-TEC-5
  //     fail-high convention as get_element_box/scroll_element_into_view -- the id is not
  //     found, or the resolved element itself is authored `focus: none;`.
  // PT: Dá foco de entrada a um elemento, programaticamente (L1.17-FOCUS -- consome a semente
  //     GAP-4 documentada em docs/embed-integration.md seção 5: hosts cujo MODELO é dono da
  //     seleção, ex.: um menu de jogo dirigido por data-binding em vez da navegação Tab/setas
  //     própria do RmlUi). Encapsula Rml::Element::Focus(false).
  //     IMPORTANTE: controlado pela propriedade RCSS `focus` do elemento (keyword
  //     `auto`|`none`), cujo default registrado no RmlUi é `auto` -- ou seja, por padrão TODO
  //     elemento (incluindo uma `<div>` lisa) aceita esta chamada, A MENOS QUE autorado
  //     explicitamente com `focus: none;`. NÃO é a mesma propriedade que `tab-index` (só a
  //     ORDEM de travessia por tecla Tab, default `none`, sem nenhum efeito neste método) --
  //     não há equivalente RmlUi/glintfx da regra implícita do HTML de "só elementos com
  //     tabindex são focáveis". Ver o doc-comment de set_focus em glintfx/src/bootstrap.hpp
  //     para o contrato completo confirmado contra o source pinado do RmlUi 6.3.
  //     Retorna false quando nenhum documento estiver carregado, id for nulo/vazio ("") --
  //     mesma convenção fail-high AUD-TEC-5 de get_element_box/scroll_element_into_view -- o
  //     id não for encontrado, ou o próprio elemento resolvido estiver autorado com `focus:
  //     none;`.
  bool set_focus(const char* id) const;

  // EN: Remove input focus (L1.17-FOCUS). Wraps Rml::Context::GetFocusElement() +
  //     Rml::Element::Blur(). Returns false when no document is loaded. Returns TRUE
  //     (deliberate idempotent no-op) when a document IS loaded but nothing is currently
  //     focused -- there is nothing to undo, so "cleared" already holds. See
  //     glintfx/src/bootstrap.hpp's clear_focus doc-comment for the full contract, including
  //     the context-wide (not document-scoped) semantics of RmlUi focus.
  // PT: Remove o foco de entrada (L1.17-FOCUS). Encapsula Rml::Context::GetFocusElement() +
  //     Rml::Element::Blur(). Retorna false quando nenhum documento estiver carregado. Retorna
  //     TRUE (no-op idempotente deliberado) quando um documento ESTÁ carregado mas nada está
  //     focado no momento -- não há nada para desfazer, então "limpo" já vale. Ver o
  //     doc-comment de clear_focus em glintfx/src/bootstrap.hpp para o contrato completo,
  //     incluindo a semântica de foco do RmlUi ser do contexto inteiro (não restrita ao
  //     documento).
  bool clear_focus() const;

  // -------------------------------------------------------------------------
  // EN: DOM read/write by id (L1.16-DOMRW, consumes AUD-PUB-6(d) -- "imperative setters by
  //     id", the seed logged in the INBOX section of TODO.md). Mirrors the guard shape of
  //     get_element_box/scroll_element_into_view/set_focus above: false when no document is
  //     loaded, or `id` is null/empty ("") -- AUD-TEC-5 fail-high. No coordinate-space
  //     translation applies to any of these (unlike get_element_box/set_click_info_callback's
  //     x/y) -- they operate on text/classes/properties, not geometry, so the sub-viewport
  //     offset from set_viewport(x,y,w,h,target_h) is irrelevant here. Parity with
  //     App::set_text/add_class/remove_class/set_property (same signatures).
  // PT: Leitura/escrita de DOM por id (L1.16-DOMRW, consome AUD-PUB-6(d) -- "setters
  //     imperativos por id", a semente registrada na seção INBOX do TODO.md). Espelha o
  //     formato de guard de get_element_box/scroll_element_into_view/set_focus acima: false
  //     quando nenhum documento estiver carregado, ou `id` for nulo/vazio ("") -- fail-high
  //     AUD-TEC-5. Nenhuma tradução de espaço de coordenada se aplica a nenhum destes
  //     (diferente do x/y de get_element_box/set_click_info_callback) -- operam sobre
  //     texto/classes/propriedades, não geometria, então o offset de sub-viewport de
  //     set_viewport(x,y,w,h,target_h) é irrelevante aqui. Paridade com
  //     App::set_text/add_class/remove_class/set_property (mesmas assinaturas).
  // -------------------------------------------------------------------------

  // EN: Replace an element's text content by id. `text` is treated as LITERAL TEXT, NEVER
  //     markup: `&`, `<`, `>`, `"` are escaped (`Rml::StringUtilities::EncodeRml`, the SAME
  //     encoder RmlUi's own text-node serialisation uses internally, confirmed against the
  //     pinned RmlUi 6.3 source) BEFORE the string reaches `Rml::Element::SetInnerRML()`. This
  //     is a HARD guarantee, not best-effort: a host that forwards untrusted text (chat, a
  //     player-chosen display name, any user-generated string) can NEVER use this call to
  //     inject RML/tags into the document -- the literal string `"<b>hi</b>"` renders as the
  //     visible text `<b>hi</b>`, it does not become a bold element. Skipping this escape would
  //     be the RML-injection equivalent of building a SQL query by string concatenation. A null
  //     `text` is normalised to `""` (same convention as `set_string`'s data-model sibling),
  //     never rejected -- an empty text content is a legitimate value.
  //     Returns `false` when no document is loaded, `id` is null/empty (`""`) or not found --
  //     same `AUD-TEC-5` fail-high convention as `get_element_box`/`set_focus` above.
  //     `SetInnerRML` itself returns `void` (there is no RmlUi-level parse outcome for plain
  //     text, unlike `set_property` below), so `true` here means only "the id was valid and the
  //     element was found", not a parse result.
  // PT: Substitui o conteúdo de texto de um elemento por id. `text` é tratado como TEXTO
  //     LITERAL, NUNCA markup: `&`, `<`, `>`, `"` são escapados (`Rml::StringUtilities::
  //     EncodeRml`, o MESMO encoder que a própria serialização de nós de texto do RmlUi usa
  //     internamente, confirmado contra o source pinado do RmlUi 6.3) ANTES da string chegar em
  //     `Rml::Element::SetInnerRML()`. Esta é uma garantia FORTE, não best-effort: um host que
  //     encaminha texto não confiável (chat, nome de exibição escolhido pelo jogador, qualquer
  //     string gerada por usuário) NUNCA consegue usar esta chamada para injetar RML/tags no
  //     documento -- a string literal `"<b>oi</b>"` renderiza como o texto visível `<b>oi</b>`,
  //     não vira um elemento em negrito. Pular este escape seria o equivalente de injeção de
  //     RML a construir uma query SQL por concatenação de string. Um `text` nulo é normalizado
  //     para `""` (mesma convenção do irmão `set_string` do data-model), nunca rejeitado --
  //     conteúdo de texto vazio é um valor legítimo.
  //     Retorna `false` quando nenhum documento estiver carregado, `id` for nulo/vazio (`""`)
  //     ou não encontrado -- mesma convenção fail-high `AUD-TEC-5` de `get_element_box`/
  //     `set_focus` acima. O próprio `SetInnerRML` retorna `void` (não há resultado de parse em
  //     nível RmlUi para texto simples, diferente do `set_property` abaixo), então `true` aqui
  //     significa só "o id era válido e o elemento foi encontrado", não um resultado de parse.
  bool set_text(const char* id, const char* text) const;

  // EN: Add a CSS class on an element by id -- wraps `Rml::Element::SetClass(cls, true)`, which
  //     returns `void` (no RmlUi-level parse outcome, unlike `set_property` below). Returns
  //     `false` when no document is loaded, `id`/`cls` is null/empty (`""`) or `id` is not
  //     found -- `AUD-TEC-5` fail-high, applied to BOTH parameters (an empty class name is a
  //     structurally-invalid caller input -- e.g. a computed class string that resolved empty
  //     -- rejected the same way an empty id is).
  // PT: Adiciona uma classe CSS num elemento por id -- encapsula `Rml::Element::SetClass(cls,
  //     true)`, que retorna `void` (nenhum resultado de parse em nível RmlUi, diferente do
  //     `set_property` abaixo). Retorna `false` quando nenhum documento estiver carregado,
  //     `id`/`cls` for nulo/vazio (`""`) ou `id` não for encontrado -- fail-high `AUD-TEC-5`,
  //     aplicado aos DOIS parâmetros (um nome de classe vazio é entrada estruturalmente
  //     inválida do chamador -- ex.: uma string de classe computada que resolveu vazia --
  //     rejeitada do mesmo jeito que um id vazio).
  bool add_class(const char* id, const char* cls) const;

  // EN: Remove a CSS class from an element by id -- wraps `Rml::Element::SetClass(cls,
  //     false)`. Same `id`/`cls` null/empty guard as `add_class` above. Removing a class the
  //     element never had is a safe no-op inside RmlUi itself (`SetClass`'s `false` branch is a
  //     set-difference operation), so this still returns `true` in that case -- the guard here
  //     only rejects a structurally-invalid `cls`, not a semantically-redundant one.
  // PT: Remove uma classe CSS de um elemento por id -- encapsula `Rml::Element::SetClass(cls,
  //     false)`. Mesmo guard de `id`/`cls` nulo/vazio do `add_class` acima. Remover uma classe
  //     que o elemento nunca teve é um no-op seguro dentro do próprio RmlUi (o ramo `false` de
  //     `SetClass` é uma operação de diferença de conjunto), então ainda retorna `true` nesse
  //     caso -- o guard aqui só rejeita um `cls` estruturalmente inválido, não um
  //     semanticamente redundante.
  bool remove_class(const char* id, const char* cls) const;

  // EN: Set a single inline RCSS property by name on an element by id -- wraps
  //     `Rml::Element::SetProperty(name, value)`, which (unlike `SetClass`/`SetInnerRML` above)
  //     DOES return a real `bool`: RmlUi's own property parser rejects an unparseable `value`
  //     for the named property (e.g. `set_property(id, "color", "not-a-color")`) and
  //     `SetProperty` propagates that rejection as `false` -- this method forwards RmlUi's own
  //     bool unchanged, it does not add a separate glintfx-level validation pass. `id`/`prop`
  //     null or empty (`""`) is rejected (`false`) BEFORE touching RmlUi -- same `AUD-TEC-5`
  //     discipline as `id`/`cls` above -- an empty property NAME cannot resolve to any real
  //     RCSS property, structurally invalid, distinct from an unparseable `value` (which IS
  //     forwarded to RmlUi's own parser rather than pre-rejected here, since only RmlUi itself
  //     knows which values are valid for which properties). `value` null is normalised to `""`
  //     (same convention as `set_text` above) and forwarded as-is.
  // PT: Define uma única propriedade RCSS inline por nome num elemento por id -- encapsula
  //     `Rml::Element::SetProperty(name, value)`, que (diferente de `SetClass`/`SetInnerRML`
  //     acima) DE FATO retorna um `bool` real: o próprio parser de propriedade do RmlUi rejeita
  //     um `value` que não parseia para a propriedade nomeada (ex.: `set_property(id, "color",
  //     "not-a-color")`) e `SetProperty` propaga essa rejeição como `false` -- este método
  //     repassa o próprio bool do RmlUi sem mudança, não adiciona uma passada de validação
  //     separada em nível glintfx. `id`/`prop` nulo ou vazio (`""`) é rejeitado (`false`) ANTES
  //     de tocar o RmlUi -- mesma disciplina `AUD-TEC-5` de `id`/`cls` acima -- um NOME de
  //     propriedade vazio não resolve para nenhuma propriedade RCSS real, estruturalmente
  //     inválido, distinto de um `value` que não parseia (que É encaminhado ao próprio parser
  //     do RmlUi em vez de pré-rejeitado aqui, já que só o próprio RmlUi sabe quais valores são
  //     válidos para quais propriedades). `value` nulo é normalizado para `""` (mesma convenção
  //     do `set_text` acima) e encaminhado como está.
  bool set_property(const char* id, const char* prop, const char* value) const;

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1) — parity with App. Call order: create_data_model
  //     -> bind_* -> load() -> set_*. All methods forward to Engine; the Engine
  //     enforces the ordering constraint (bind_* after load() returns false).
  //     API uses only plain C types (const char*, double, bool, size_t).
  // PT: API de data-model (T1) — paridade com App. Ordem de chamada:
  //     create_data_model -> bind_* -> load() -> set_*(). Todos os métodos
  //     encaminham ao Engine; o Engine enforça a restrição de ordem (bind_*
  //     após load() retorna false). API usa apenas tipos C simples
  //     (const char*, double, bool, size_t).
  // -------------------------------------------------------------------------

  // EN: Create a data model. Call before bind_* and before load(). LIMIT: at most ONE data
  //     model per UiLayer instance -- a second create_data_model() call (any name, including a
  //     different one) returns false and is a silent no-op; the first model stays the only one
  //     bound (DataBinder's `created` flag, checked-before-mutate). Not currently surfaced to
  //     the caller other than the false return -- if multiple independent models are needed,
  //     bind all variables under the ONE model created here. Exercised end-to-end (embed path)
  //     by data_model_embed_sanity.cpp. AUD-TEC-7 (2026-07-08).
  // PT: Cria um data model. Chamar antes de bind_* e antes de load(). LIMITE: no máximo UM
  //     data model por instância de UiLayer -- uma segunda chamada a create_data_model()
  //     (qualquer nome, mesmo diferente) retorna false e é um no-op silencioso; o primeiro
  //     modelo permanece o único ligado (flag `created` do DataBinder, check-before-mutate).
  //     Não é sinalizado ao chamador além do retorno false -- se múltiplos modelos
  //     independentes forem necessários, ligue todas as variáveis sob o ÚNICO modelo criado
  //     aqui. Exercitado ponta-a-ponta (caminho embed) por data_model_embed_sanity.cpp.
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

  // EN: Read-back the CURRENT value of a bound cell by key (L1.16-DOMRW, consumes
  //     AUD-PUB-6(e) -- the data-model was write-only before this: a host could push values in
  //     via `set_*`, but had no way to read back a value the UI itself, or the user through a
  //     bound control (e.g. an `<input data-value>`), wrote into the model -- "two-way binding
  //     invisible to the host", per the roadmap item). Correct by construction, not by
  //     coincidence: `Rml::DataModelConstructor::Bind(name, ptr)` (confirmed against
  //     `Include/RmlUi/Core/DataModelHandle.h` in the pinned RmlUi 6.3 source) registers a
  //     `DataVariable` wrapping the SAME pointer glintfx's internal `DataBinder` stores the
  //     bound cell at (`static_cast<void*>(ptr)`) -- any bidirectional RmlUi-driven write (a
  //     data-view setter firing from user interaction) lands directly in THAT cell, so reading
  //     it here IS reading the model's live, current value, with no extra RmlUi call needed.
  //     Returns `false` (`out` left untouched) when the key was never bound via the matching
  //     `bind_number`/`bind_string`/`bind_bool` -- mirrors `set_*`'s "no-op when key unknown"
  //     convention, made observable here since there is an out-param to (not) write. No
  //     document/`load()`-ordering guard of its own beyond that -- a key that IS bound is
  //     readable at any point after the matching `bind_*` succeeds, before or after `load()`.
  //     `std::string` at this boundary is an intentional, deliberate exception to the
  //     plain-C-types convention the rest of this data-model API follows (`const char*`
  //     in/out, `bool` for `bind_bool`, `Rml::String v.s. std::string` for lists): a `const
  //     char*` OUT-parameter/return here would have to point into memory this class owns and
  //     controls the lifetime of (the internal `DataBinder` cell, or the temporary
  //     `Rml::String` `SetProperty`/etc. above return by value) -- exactly the class of bug the
  //     `v0.4.1` heap-use-after-free audit finding was about (a raw pointer into internal
  //     storage whose lifetime the caller cannot reason about). Returning by
  //     value/reference-out into a caller-owned `std::string` sidesteps that lifetime question
  //     entirely -- there is no dangling-pointer failure mode to introduce. Parity with
  //     `App::get_number`/`get_string`/`get_bool` (same signatures).
  // PT: Lê de volta o valor CORRENTE de uma célula ligada por chave (L1.16-DOMRW, consome
  //     AUD-PUB-6(e) -- o data-model era write-only antes disto: um host podia empurrar valores
  //     via `set_*`, mas não tinha como ler de volta um valor que a própria UI, ou o usuário
  //     através de um controle ligado (ex.: um `<input data-value>`), escreveu no modelo --
  //     "two-way binding invisível ao host", conforme o item de roadmap). Correto por
  //     construção, não por coincidência: `Rml::DataModelConstructor::Bind(name, ptr)`
  //     (confirmado contra `Include/RmlUi/Core/DataModelHandle.h` no source pinado do RmlUi
  //     6.3) registra uma `DataVariable` envolvendo o MESMO ponteiro onde o `DataBinder`
  //     interno da glintfx guarda a célula ligada (`static_cast<void*>(ptr)`) -- qualquer
  //     escrita bidirecional dirigida pelo RmlUi (um setter de data-view disparando por
  //     interação do usuário) cai direto NESSA célula, então lê-la aqui É ler o valor vivo e
  //     corrente do modelo, sem chamada extra ao RmlUi.
  //     Retorna `false` (`out` intocado) quando a chave nunca foi ligada pelo `bind_number`/
  //     `bind_string`/`bind_bool` correspondente -- espelha a convenção "no-op quando chave
  //     desconhecida" do `set_*`, tornada observável aqui por haver um out-param a (não)
  //     escrever. Sem guard próprio de documento/ordem-de-`load()` além disso -- uma chave que
  //     ESTÁ ligada é legível a qualquer momento após o `bind_*` correspondente ter sucesso,
  //     antes ou depois de `load()`.
  //     `std::string` nesta fronteira é uma exceção intencional e deliberada à convenção de
  //     tipos C simples que o resto desta API de data-model segue (`const char*` in/out, `bool`
  //     para `bind_bool`): um OUT-parâmetro/retorno `const char*` aqui teria que apontar para
  //     memória cujo lifetime esta classe possui e controla (a célula interna do `DataBinder`,
  //     ou o `Rml::String` temporário que `SetProperty`/etc. acima retornam por valor) --
  //     exatamente a classe de bug do achado de auditoria de heap-use-after-free da `v0.4.1`
  //     (um ponteiro cru para storage interno cujo lifetime o chamador não consegue raciocinar
  //     sobre). Retornar por valor/referência-out num `std::string` de propriedade do chamador
  //     evita essa questão de lifetime inteiramente -- não há modo de falha de
  //     ponteiro-pendurado a introduzir. Paridade com `App::get_number`/`get_string`/`get_bool`
  //     (mesmas assinaturas).
  bool get_number(const char* key, double& out) const;
  bool get_string(const char* key, std::string& out) const;
  bool get_bool  (const char* key, bool& out) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
