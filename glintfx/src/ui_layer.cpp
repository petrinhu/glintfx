// SPDX-License-Identifier: Apache-2.0
// EN: UiLayer implementation — embed/guest facade that attaches the RmlUi+GL3 engine to
//     a GL context owned by the host. Owns a SystemClock (no GLFW) and an Engine.
// PT: Implementação do UiLayer — fachada embed/guest que anexa o motor RmlUi+GL3 a um
//     contexto GL do host. Possui SystemClock (sem GLFW) e Engine.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl_loader.h must be included before any other OpenGL header (defines GL function pointers).
// PT: gl_loader.h deve ser incluído antes de qualquer outro header OpenGL (define ponteiros de função GL).
#include "gl_loader.h"

#include <glintfx/ui_layer.hpp>
#include "engine.hpp"
#include "rml/system_clock.hpp"
// EN: RMLX-0/F3 -- glintfx::log_warn/log_error (glintfx/log.hpp) replace the direct
//     Rml::Log::Message(LT_WARNING/LT_ERROR, ...) calls this file used before this fatia (same
//     final destination -- a host's own installed sink, or the fprintf(stderr, ...) default --
//     but routed through this library's OWN public log surface instead of RmlUi's, same move F2
//     already made for engine.cpp). The Resize branch below (process_event()) no longer needs a
//     raw Rml::Context* either -- it now drives the viewport resize through Engine::set_viewport,
//     same as every other Engine caller -- so this file drops its RmlUi header include entirely,
//     closing the LAST direct RmlUi contact point outside src/rml/ (RMLX-0's own goal).
// PT: RMLX-0/F3 -- glintfx::log_warn/log_error (glintfx/log.hpp) substituem as chamadas diretas
//     Rml::Log::Message(LT_WARNING/LT_ERROR, ...) que este arquivo usava antes desta fatia (mesmo
//     destino final -- o sink próprio instalado por um host, ou o default fprintf(stderr, ...) --
//     mas roteado pela superfície de log PRÓPRIA desta biblioteca em vez da do RmlUi, mesmo
//     movimento que a F2 já fez para engine.cpp). O branch de Resize abaixo (process_event())
//     também deixa de precisar de um Rml::Context* cru -- agora ele conduz o redimensionamento de
//     viewport via Engine::set_viewport, igual a todo outro chamador do Engine -- então este
//     arquivo deixa cair o próprio include de header do RmlUi por completo, fechando o ÚLTIMO ponto de
//     contato direto com o RmlUi fora de src/rml/ (o próprio objetivo da RMLX-0).
#include <glintfx/log.hpp>
#include <string> // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).

namespace glintfx {

// EN: Sane ceiling for viewport dimensions/offsets (AUD-TEC-4). No real display or letterbox
//     layout needs more than this; the point is to keep `target_h - y - h` (set_viewport's
//     gl_offset_y computation, below) comfortably inside int range so it can never
//     signed-overflow (UB) even with adversarial input close to INT_MAX.
// PT: Teto são para dimensões/offsets de viewport (AUD-TEC-4). Nenhum display ou layout de
//     letterbox real precisa de mais que isto; o objetivo é manter `target_h - y - h` (o
//     cálculo de gl_offset_y do set_viewport, abaixo) folgado dentro do range de int, para que
//     nunca dê overflow com sinal (UB) mesmo com entrada adversarial perto de INT_MAX.
constexpr int kMaxViewportDim = 32768;

struct UiLayer::Impl {
  SystemClock clock; // EN: minimal RmlUi SystemInterface (clock only, no GLFW).
                     // PT: SystemInterface mínimo do RmlUi (só relógio, sem GLFW).
  Engine engine;
  int w = 0;
  int h = 0;
  // EN: F3 (v0.2.5) -- see set_viewport() doc comments in ui_layer.hpp for the full contract.
  //     x, y: public, top-down window-space offset (mirrors UiEvent's convention).
  //     target_h: last target_h given to the 5-arg overload; re-used by the Resize event
  //           handler below to keep gl_offset_y consistent when content h changes via
  //           process_event() instead of via an explicit set_viewport() call.
  //     gl_offset_x/y: OpenGL-native (bottom-left) offset, PRE-COMPUTED here and passed
  //           through unchanged every frame in render() -- render() itself does zero math.
  //     letterbox_mode: false = legacy 2-arg path (offset always (0,0), Resize never touches
  //           it). true = 5-arg path -- Resize recomputes gl_offset_y (not gl_offset_x, x
  //           does not depend on h).
  // PT: F3 (v0.2.5) -- ver doc-comments de set_viewport() em ui_layer.hpp pro contrato
  //     completo. x, y: offset público, espaço-janela top-down (espelha a convenção do
  //     UiEvent). target_h: último target_h passado à sobrecarga de 5 args; reusado pelo
  //     handler de evento Resize abaixo pra manter gl_offset_y consistente quando o h de
  //     conteúdo muda via process_event() em vez de via set_viewport() explícito.
  //     gl_offset_x/y: offset nativo do OpenGL (inferior-esquerda), PRÉ-CALCULADO aqui e
  //     repassado sem mudança a cada frame em render() -- o render() em si não faz conta
  //     nenhuma. letterbox_mode: false = caminho legado de 2 args (offset sempre (0,0),
  //     Resize nunca o toca). true = caminho de 5 args -- Resize recalcula gl_offset_y (não
  //     gl_offset_x, x não depende de h).
  int x = 0, y = 0, target_h = 0;
  int gl_offset_x = 0, gl_offset_y = 0;
  bool letterbox_mode = false;
  bool ok = false;
};

UiLayer::UiLayer(Config cfg) : impl_(std::make_unique<Impl>()) {
  // EN: UILAYER-CTOR-GUARD (bugfix, W22 S2) -- validate cfg.logical_width/logical_height BEFORE
  //     they ever reach Engine::attach()/Rml::CreateContext, mirroring the SAME range check
  //     set_viewport() enforces (kMaxViewportDim ceiling, w<=0||h<=0 floor) so both entry points
  //     to viewport dimensions reject the identical range -- this constructor was the
  //     unvalidated twin: set_viewport()'s guards (below) already existed, this one did not.
  //     MEASURED gap this closes: without this guard, UiLayerConfig{.logical_width = 0} reached
  //     Rml::CreateContext("main", {0, h}) and RmlUi swallowed it -- ok() came back TRUE, leaving
  //     the object usable in a state set_viewport() would never permit. The easiest way to hit
  //     this in practice: a host reading a not-yet-sized window under Wayland (reports 0x0 while
  //     minimized) straight into UiLayerConfig.
  //     Policy (team lead's call, not this fatia's to choose unilaterally): the house's fail-high
  //     convention elsewhere is "reject, keep the previous value, log a warning" -- but a
  //     CONSTRUCTOR has no previous value to fall back on. The chosen shape here is
  //     `ok() == false` + LT_ERROR (not LT_WARNING): unlike a silently-ignored resize, an invalid
  //     ctor leaves the WHOLE object unusable, a failure the host cannot help but notice via
  //     ok() -- see class-level doc-comment in ui_layer.hpp for the constructor's full contract.
  //     impl_->w/h are deliberately left at their Impl-default (0) rather than storing the
  //     rejected input -- no other method reads them while ok() is false (every one of them
  //     starts with `if (!ready()) return`), so there is nothing for a stale value to corrupt.
  // PT: UILAYER-CTOR-GUARD (bugfix, W22 S2) -- valida cfg.logical_width/logical_height ANTES de
  //     chegarem a Engine::attach()/Rml::CreateContext, espelhando a MESMA checagem de range que
  //     set_viewport() enforça (teto kMaxViewportDim, piso w<=0||h<=0) para que as duas portas de
  //     entrada de dimensão de viewport rejeitem o range idêntico -- este construtor era o gêmeo
  //     não validado: os guards de set_viewport() (abaixo) já existiam, este não.
  //     Lacuna MEDIDA que isto fecha: sem este guard, UiLayerConfig{.logical_width = 0} chegava a
  //     Rml::CreateContext("main", {0, h}) e o RmlUi engolia -- ok() voltava TRUE, deixando o
  //     objeto utilizável num estado que set_viewport() jamais permitiria. A forma mais fácil de
  //     cair nisto na prática: um host lendo uma janela ainda sem tamanho sob Wayland (reporta
  //     0x0 enquanto minimizada) direto em UiLayerConfig.
  //     Política (decisão do team lead, não desta fatia decidir unilateralmente): a convenção
  //     fail-high da casa em outros pontos é "rejeitar, manter o valor anterior, logar aviso" --
  //     mas um CONSTRUTOR não tem valor anterior para cair. A forma escolhida aqui é
  //     `ok() == false` + LT_ERROR (não LT_WARNING): diferente de um resize ignorado em silêncio,
  //     um ctor inválido deixa o OBJETO INTEIRO inutilizável, uma falha que o host não tem como
  //     deixar de notar via ok() -- ver o comentário de nível de classe em ui_layer.hpp pro
  //     contrato completo do construtor.
  //     impl_->w/h são deliberadamente deixados no default do Impl (0) em vez de armazenar a
  //     entrada rejeitada -- nenhum outro método os lê enquanto ok() for false (todos começam
  //     com `if (!ready()) return`), então não há valor obsoleto para corromper nada.
  if (cfg.logical_width <= 0 || cfg.logical_height <= 0 ||
      cfg.logical_width > kMaxViewportDim || cfg.logical_height > kMaxViewportDim) {
    glintfx::log_error(
        "UiLayer(logical_width=%d, logical_height=%d) rejected -- dimensions must "
        "be positive and at most %d; ok() will return false.",
        cfg.logical_width, cfg.logical_height, kMaxViewportDim);
    return;
  }

  impl_->w = cfg.logical_width;
  impl_->h = cfg.logical_height;

  // EN: Load GL function pointers against the host's CURRENT context.
  //     glx_gl_load() is idempotent within one process — a repeat call just re-resolves
  //     and overwrites the same ~344 pointers (cheap, no allocation kept around).
  //     Skip when the host already claims the loader is populated -- SEED-LOADGL-NOME
  //     (2026-08-04): EITHER field asking to skip is enough, so the combined guard is
  //     `load_gl && !assume_gl_loaded`, not a straight read of one field. This is the ONE
  //     internal site that still reads the deprecated `load_gl` (every other internal caller
  //     of a UiLayerConfig uses only assume_gl_loaded or the fields' own defaults) -- the
  //     pragma below silences this file warning about a deprecation this file itself is the
  //     one keeping alive on purpose, for the one version load_gl stays functional.
  //     GLLOADER-HOST (2026-08-04): when the load path IS active and the host supplied
  //     `cfg.gl_proc_resolver`, that resolver -- not glintfx's own glX/EGL/dlsym chain --
  //     populates the table (glx_gl_load_with(), gl_loader.h/.c; see UiLayerConfig's own
  //     doc-comment for the full three-way rule). When the load path is being SKIPPED
  //     (assume_gl_loaded, or the deprecated load_gl=false) but a resolver was ALSO supplied,
  //     that is a contradiction from the caller -- the skip claim wins, the resolver is never
  //     called, and a warning names why.
  // PT: Carrega ponteiros de função GL contra o contexto CORRENTE do host.
  //     glx_gl_load() é idempotente dentro de um processo — uma chamada repetida apenas
  //     re-resolve e sobrescreve os mesmos ~344 ponteiros (barato, sem alocação retida).
  //     Pular quando o host já alega que o loader está populado -- SEED-LOADGL-NOME
  //     (2026-08-04): QUALQUER um dos dois campos pedindo pra pular já basta, então a guarda
  //     combinada é `load_gl && !assume_gl_loaded`, não a leitura direta de um campo só. Este é
  //     o ÚNICO sítio interno que ainda lê o `load_gl` deprecated (todo outro chamador interno
  //     de UiLayerConfig usa só assume_gl_loaded ou os próprios defaults dos campos) -- o pragma
  //     abaixo silencia este arquivo avisando de uma depreciação que este mesmo arquivo é quem
  //     mantém viva de propósito, pela uma versão em que load_gl segue funcional.
  //     GLLOADER-HOST (2026-08-04): quando o caminho de load ESTÁ ativo e o host forneceu
  //     `cfg.gl_proc_resolver`, é esse resolvedor -- não a própria cadeia glX/EGL/dlsym da
  //     glintfx -- que popula a tabela (glx_gl_load_with(), gl_loader.h/.c; ver o próprio
  //     doc-comment de UiLayerConfig pra regra completa de três vias). Quando o caminho de load
  //     está sendo PULADO (assume_gl_loaded, ou o load_gl=false deprecated) mas um resolvedor
  //     TAMBÉM foi fornecido, isso é uma contradição do chamador -- a alegação de pular vence,
  //     o resolvedor nunca é chamado, e um aviso nomeia o motivo.
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  const bool glloader_host_load_path_active = cfg.load_gl && !cfg.assume_gl_loaded;
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  if (glloader_host_load_path_active) {
    if (cfg.gl_proc_resolver) {
      if (glx_gl_load_with(cfg.gl_proc_resolver) != 0) {
        glintfx::log_error(
            "UiLayer(...): gl_proc_resolver failed to resolve one or more core GL "
            "symbols (origin: host resolver); ok() will return false.");
        return;
      }
    } else {
      if (glx_gl_load() != 0) return;
    }
  } else if (cfg.gl_proc_resolver) {
    glintfx::log_warn(
        "UiLayer(...): gl_proc_resolver was set but the loader is being skipped "
        "(assume_gl_loaded=true, or the deprecated load_gl=false) -- the resolver "
        "is ignored; the caller's already-loaded claim takes precedence.");
  }

  // EN: AUD-UILAYER-MOVEDFROM (W26) -- local reference `impl`, used for the `ok` writes below.
  //     impl_ is guaranteed non-null here (just constructed above), so there is nothing to
  //     null-check in this constructor -- using the reference here keeps the null-safe
  //     dereference pattern this file used to spell out at every entry point reserved for
  //     exactly ONE place: ready()'s own definition, further down (right before UiLayer::ok(),
  //     which now just forwards to it).
  // PT: AUD-UILAYER-MOVEDFROM (W26) -- referência local `impl`, usada para as escritas de `ok`
  //     abaixo. impl_ é garantidamente não-nulo aqui (acabou de ser construído acima), então
  //     não há nada para checar contra nulo neste construtor -- usar a referência aqui mantém
  //     o padrão de derreferência null-safe que este arquivo costumava soletrar em todo ponto
  //     de entrada reservado para exatamente UM lugar: a própria definição de ready(), mais
  //     adiante (logo antes de UiLayer::ok(), que agora só repassa para ele).
  Impl& impl = *impl_;
  impl.ok = impl.engine.attach(&impl.clock, impl.w, impl.h, cfg.font_engine);
  // EN: Apply the initial dp_ratio to the newly created context.
  //     SetDensityIndependentPixelRatio is idempotent when value equals the context default
  //     (1.0f), so always calling it here is safe and makes the intent explicit.
  // PT: Aplica o dp_ratio inicial ao contexto recém-criado.
  //     SetDensityIndependentPixelRatio é idempotente quando o valor iguala o padrão do
  //     contexto (1.0f), então chamá-lo sempre aqui é seguro e deixa a intenção explícita.
  if (impl.ok) impl.engine.set_dp_ratio(cfg.dp_ratio);
}

UiLayer::~UiLayer() = default;

UiLayer::UiLayer(UiLayer&&) noexcept = default;
UiLayer& UiLayer::operator=(UiLayer&&) noexcept = default;

bool UiLayer::ready() const noexcept {
  return impl_ && impl_->ok;
}

bool UiLayer::ok() const noexcept {
  return ready();
}

bool UiLayer::load(const char* rml_path) {
  if (!ready()) return false;
  return impl_->engine.load(rml_path);
}

void UiLayer::set_viewport(int w, int h) {
  if (!ready()) return;
  // EN: Input-hardening (audit, v0.3.0). w/h are viewport dimensions fed to
  //     Rml::Context::SetDimensions; a zero/negative dimension degenerates the layout engine.
  //     Skip silently and keep the previous viewport -- this replicates the exact guard that
  //     already lives in process_event(Resize) (`if (ev.width <= 0 || ev.height <= 0) break;`);
  //     the two set_viewport overloads were the ones that lacked it. x/y/target_h are NOT
  //     validated: a legitimate letterbox offset can be negative.
  // PT: Hardening de entrada (auditoria, v0.3.0). w/h são dimensões de viewport passadas a
  //     Rml::Context::SetDimensions; uma dimensão zero/negativa degenera o motor de layout.
  //     Ignora silenciosamente e mantém o viewport anterior -- replica o guard exato que já
  //     existe em process_event(Resize) (`if (ev.width <= 0 || ev.height <= 0) break;`); as duas
  //     sobrecargas de set_viewport eram as que não o tinham. x/y/target_h NÃO são validados: um
  //     offset de letterbox legítimo pode ser negativo.
  // EN: Guard (AUD-TEC-4): also cap w/h at a sane ceiling -- kMaxViewportDim is shared with the
  //     5-arg overload below so both reject the same range.
  // PT: Guard (AUD-TEC-4): também limita w/h a um teto são -- kMaxViewportDim é compartilhado
  //     com a sobrecarga de 5 args abaixo para que ambas rejeitem o mesmo range.
  // EN: QW-GUARDLOG (v0.18.1) -- this pre-guard short-circuits BEFORE Engine::set_viewport is ever
  //     called, so logging only at that common Engine point (as done for App's direct callers)
  //     would leave every embed-mode host (UiLayer's own callers) silently unwarned -- this is the
  //     actually-reachable rejection point for them, so it gets its own warning too.
  // PT: QW-GUARDLOG (v0.18.1) -- este pre-guard intercepta ANTES de Engine::set_viewport sequer
  //     ser chamado, então logar só naquele ponto comum do Engine (como feito para os callers
  //     diretos do App) deixaria todo host em embed mode (callers do próprio UiLayer) sem aviso
  //     algum em silêncio -- este é o ponto de rejeição de fato alcançável por eles, então também
  //     ganha aviso próprio.
  if (w <= 0 || h <= 0 || w > kMaxViewportDim || h > kMaxViewportDim) {
    glintfx::log_warn(
        "set_viewport(%d, %d) ignored -- dimensions must be positive and at most %d "
        "(previous viewport kept).",
        w, h, kMaxViewportDim);
    return;
  }
  impl_->w = w;
  impl_->h = h;
  impl_->x = impl_->y = impl_->gl_offset_x = impl_->gl_offset_y = 0;
  impl_->target_h = h;
  impl_->letterbox_mode = false;
  impl_->engine.set_viewport(w, h);
}

void UiLayer::set_viewport(int x, int y, int w, int h, int target_h) {
  if (!ready()) return;
  // EN: Same w/h guard as the 2-arg overload above (input-hardening audit, v0.3.0). Only w/h are
  //     checked here -- x/y (letterbox origin) can legitimately be any value including negative
  //     offsets. target_h (total window height) must stay positive, see AUD-TEC-4 below. Keeps
  //     the previous viewport on invalid dimensions.
  // PT: Mesmo guard de w/h da sobrecarga de 2 args acima (auditoria de hardening, v0.3.0). Aqui
  //     só w/h são checados -- x/y (origem do letterbox) podem legitimamente ser qualquer valor,
  //     incluindo offsets negativos. target_h (altura total da janela) precisa continuar
  //     positivo, ver AUD-TEC-4 abaixo. Mantém o viewport anterior em dimensões inválidas.
  // EN: Guard (AUD-TEC-4): x/y/target_h/w/h all capped at a sane ceiling so
  //     `target_h - y - h` below cannot signed-overflow (UB) -- e.g. target_h=INT_MAX would
  //     transiently underflow int range as the subtraction proceeds. w/h and target_h must stay
  //     positive (> 0, target_h is the total window height, same rule as the 2-arg overload's
  //     w/h); only x/y may legitimately be negative (letterbox origin) but not adversarially
  //     huge in either direction.
  // PT: Guard (AUD-TEC-4): x/y/target_h/w/h todos limitados a um teto são para que
  //     `target_h - y - h` abaixo não dê overflow com sinal (UB) -- ex.: target_h=INT_MAX
  //     faria a subtração transbordar o range de int. w/h e target_h precisam continuar
  //     positivos (> 0, target_h é a altura total da janela, mesma regra do w/h da sobrecarga
  //     de 2 args); só x/y podem legitimamente ser negativos (origem de letterbox) mas não
  //     adversarialmente enormes em nenhuma direção.
  // EN: QW-GUARDLOG (v0.18.1) -- same rationale as the 2-arg overload's guard above: this
  //     pre-guard is the actually-reachable rejection point for embed-mode hosts (it never lets an
  //     out-of-range call reach Engine::set_viewport), so it warns here too.
  // PT: QW-GUARDLOG (v0.18.1) -- mesma racional do guard da sobrecarga de 2 args acima: este
  //     pre-guard é o ponto de rejeição de fato alcançável por hosts em embed mode (nunca deixa
  //     uma chamada fora do range chegar a Engine::set_viewport), então também avisa aqui.
  if (w <= 0 || h <= 0 || w > kMaxViewportDim || h > kMaxViewportDim ||
      x < -kMaxViewportDim || x > kMaxViewportDim ||
      y < -kMaxViewportDim || y > kMaxViewportDim ||
      target_h <= 0 || target_h > kMaxViewportDim) {
    glintfx::log_warn(
        "set_viewport(%d, %d, %d, %d, %d) ignored -- w/h/target_h must be positive, all five "
        "values within [-%d, %d] (previous viewport kept).",
        x, y, w, h, target_h, kMaxViewportDim, kMaxViewportDim);
    return;
  }
  impl_->x = x;
  impl_->y = y;
  impl_->w = w;
  impl_->h = h;
  impl_->target_h = target_h;
  impl_->letterbox_mode = true;
  impl_->gl_offset_x = x;
  impl_->gl_offset_y = target_h - y - h;
  impl_->engine.set_viewport(w, h);
}

void UiLayer::set_dp_ratio(float ratio) {
  if (!ready()) return;
  impl_->engine.set_dp_ratio(ratio);
}

void UiLayer::set_asset_base_url(const char* url) {
  if (!ready()) return;
  impl_->engine.set_asset_base_url(url);
}

void UiLayer::update() {
  if (!ready()) return;
  impl_->engine.update();
}

void UiLayer::render() {
  // EN: Delegate to Engine::render_compose — compose UI over the host's current FBO
  //     without clearing and without swapping buffers. GL state is saved/restored internally.
  // PT: Delega a Engine::render_compose — compõe a UI sobre o FBO corrente do host
  //     sem limpar e sem trocar buffers. Estado GL é salvo/restaurado internamente.
  if (ready())
    impl_->engine.render_compose(impl_->gl_offset_x, impl_->gl_offset_y, impl_->w, impl_->h);
}

// ---------------------------------------------------------------------------
// EN: Data-model API — forward to Engine; Engine guards the ordering constraint.
// PT: API de data-model — encaminha ao Engine; Engine enforça a restrição de ordem.
// ---------------------------------------------------------------------------

bool UiLayer::create_data_model(const char* name) {
  if (!ready()) return false;
  return impl_->engine.create_data_model(name);
}

bool UiLayer::bind_number(const char* key, double initial) {
  if (!ready()) return false;
  return impl_->engine.bind_number(key, initial);
}

bool UiLayer::bind_string(const char* key, const char* initial) {
  if (!ready()) return false;
  return impl_->engine.bind_string(key, initial);
}

bool UiLayer::bind_bool(const char* key, bool initial) {
  if (!ready()) return false;
  return impl_->engine.bind_bool(key, initial);
}

bool UiLayer::bind_list(const char* key) {
  if (!ready()) return false;
  return impl_->engine.bind_list(key);
}

void UiLayer::set_number(const char* key, double value) {
  if (!ready()) return;
  impl_->engine.set_number(key, value);
}

void UiLayer::set_string(const char* key, const char* value) {
  if (!ready()) return;
  impl_->engine.set_string(key, value);
}

void UiLayer::set_bool(const char* key, bool value) {
  if (!ready()) return;
  impl_->engine.set_bool(key, value);
}

void UiLayer::set_list(const char* key, const char* const* items, std::size_t count) {
  if (!ready()) return;
  impl_->engine.set_list(key, items, count);
}

void UiLayer::set_click_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_click_callback(std::move(cb));
}

void UiLayer::set_click_info_callback(std::function<void(const ClickInfo&)> cb) {
  if (!ready()) return;
  // EN: AUD-PUB-4 (v0.5.0): unlike set_click_callback (id-only, no coordinate translation
  //     needed), the ClickInfo the Engine/Bootstrap hand back carries x/y in CONTENT-LOCAL
  //     space (offset-free -- see Engine::set_click_info_callback's doc-comment). Wrap the
  //     host's callback in a translating lambda that adds the current sub-viewport offset
  //     (impl_->x/y) before forwarding -- the SAME translation get_element_box() applies
  //     (box.x = x + impl_->x). Captures a raw Impl* (not `this`/shared_ptr): safe because the
  //     wrapped lambda is stored inside Bootstrap::Impl::click_info_cb, which lives inside
  //     impl_->engine's Bootstrap, which is a member of THIS UiLayer::Impl -- the callback
  //     cannot outlive the Impl it points into. Reads impl_->x/y at INVOCATION time (not
  //     capture time) so a later set_viewport() call is honoured for every subsequent click.
  // PT: AUD-PUB-4 (v0.5.0): diferente de set_click_callback (só-id, sem tradução de
  //     coordenada necessária), o ClickInfo que o Engine/Bootstrap devolvem carrega x/y no
  //     espaço LOCAL DE CONTEÚDO (offset-free -- ver o doc-comment de
  //     Engine::set_click_info_callback). Envolve o callback do host numa lambda tradutora que
  //     soma o offset de sub-viewport corrente (impl_->x/y) antes de repassar -- a MESMA
  //     tradução que get_element_box() aplica (box.x = x + impl_->x). Captura um Impl* cru (não
  //     `this`/shared_ptr): seguro porque a lambda envolvida fica armazenada dentro de
  //     Bootstrap::Impl::click_info_cb, que vive dentro do Bootstrap de impl_->engine, que é
  //     membro DESTE UiLayer::Impl -- o callback não pode sobreviver ao Impl para o qual
  //     aponta. Lê impl_->x/y no momento da INVOCAÇÃO (não no momento da captura), então uma
  //     chamada posterior a set_viewport() é respeitada em todo clique subsequente.
  Impl* self = impl_.get();
  impl_->engine.set_click_info_callback(
      [self, cb = std::move(cb)](const ClickInfo& info) {
        if (!cb) return;
        ClickInfo translated = info;
        translated.x += static_cast<float>(self->x);
        translated.y += static_cast<float>(self->y);
        cb(translated);
      });
}

void UiLayer::set_scroll_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  // EN: Straight passthrough (GLINTFX-SCROLL-1 follow-up, v0.6.0) -- unlike
  //     set_click_info_callback, there is no coordinate payload here to translate by the
  //     sub-viewport offset (impl_->x/y): the callback carries only an id string.
  // PT: Repasse direto (desdobramento do GLINTFX-SCROLL-1, v0.6.0) -- diferente do
  //     set_click_info_callback, não há payload de coordenada aqui a traduzir pelo offset de
  //     sub-viewport (impl_->x/y): o callback carrega só uma string de id.
  impl_->engine.set_scroll_callback(std::move(cb));
}

// EN: L1.15-FORMEV -- straight passthrough, no coordinate translation (none of these five
//     payloads carry geometry -- id [+ value for change] only, same reasoning as
//     set_scroll_callback's lack of offset math above). See the doc-comments in ui_layer.hpp/
//     bootstrap.hpp for the full contract.
// PT: L1.15-FORMEV -- repasse direto, sem tradução de coordenada (nenhum dos cinco payloads
//     carrega geometria -- só id [+ value pro change], mesma racional da ausência de matemática
//     de offset do set_scroll_callback acima). Ver os doc-comments em ui_layer.hpp/
//     bootstrap.hpp para o contrato completo.
void UiLayer::set_change_callback(std::function<void(const char*, const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_change_callback(std::move(cb));
}

void UiLayer::set_submit_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_submit_callback(std::move(cb));
}

void UiLayer::set_focus_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_focus_callback(std::move(cb));
}

void UiLayer::set_blur_callback(std::function<void(const char*)> cb) {
  if (!ready()) return;
  impl_->engine.set_blur_callback(std::move(cb));
}

void UiLayer::set_hover_callback(std::function<void(const char*, bool)> cb) {
  if (!ready()) return;
  impl_->engine.set_hover_callback(std::move(cb));
}

ElementBox UiLayer::get_element_box(const char* id) const {
  ElementBox box;
  if (!ready()) return box;
  float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
  if (!impl_->engine.get_element_box(id, x, y, w, h)) return box;
  box.found = true;
  // EN: content-space -> window-space: add the sub-viewport offset (F3, v0.2.5 --
  //     impl_->x/y are (0,0) in legacy mode, so this is a no-op unless
  //     set_viewport(x,y,w,h,target_h) placed the sub-viewport elsewhere).
  // PT: espaço-conteúdo -> espaço-janela: soma o offset da sub-viewport (F3, v0.2.5 --
  //     impl_->x/y são (0,0) em modo legado, então é no-op a menos que
  //     set_viewport(x,y,w,h,target_h) tenha posicionado a sub-viewport em outro lugar).
  box.x = x + static_cast<float>(impl_->x);
  box.y = y + static_cast<float>(impl_->y);
  box.w = w;
  box.h = h;
  return box;
}

bool UiLayer::scroll_element_into_view(const char* id, bool align_with_top) const {
  if (!ready()) return false;
  return impl_->engine.scroll_element_into_view(id, align_with_top);
}

bool UiLayer::scroll_element_into_view(const char* id, ScrollAlign align) const {
  if (!ready()) return false;
  return impl_->engine.scroll_element_into_view(id, align);
}

bool UiLayer::get_element_scroll_top(const char* id, float& out_scroll_top) const {
  if (!ready()) return false;
  return impl_->engine.get_element_scroll_top(id, out_scroll_top);
}

bool UiLayer::get_element_scroll_height(const char* id, float& out_scroll_height) const {
  if (!ready()) return false;
  return impl_->engine.get_element_scroll_height(id, out_scroll_height);
}

bool UiLayer::get_element_client_height(const char* id, float& out_client_height) const {
  if (!ready()) return false;
  return impl_->engine.get_element_client_height(id, out_client_height);
}

bool UiLayer::set_element_scroll_top(const char* id, float scroll_top) const {
  if (!ready()) return false;
  return impl_->engine.set_element_scroll_top(id, scroll_top);
}

bool UiLayer::set_focus(const char* id) const {
  if (!ready()) return false;
  // EN: Straight passthrough (L1.17-FOCUS) -- no coordinate translation applicable, same
  //     reasoning as the scroll trio above (id-only, no x/y in either direction).
  // PT: Repasse direto (L1.17-FOCUS) -- sem tradução de coordenada aplicável, mesma
  //     racional do trio de rolagem acima (só-id, sem x/y em nenhuma direção).
  return impl_->engine.set_focus(id);
}

bool UiLayer::clear_focus() const {
  if (!ready()) return false;
  return impl_->engine.clear_focus();
}

// ---------------------------------------------------------------------------
// EN: DOM read/write by id (L1.16-DOMRW) -- straight forwards to Engine, no coordinate
//     translation applicable (same reasoning as set_focus/scroll trio above: these operate on
//     text/classes/properties, not on geometry).
// PT: Leitura/escrita de DOM por id (L1.16-DOMRW) -- repasse direto ao Engine, sem tradução de
//     coordenada aplicável (mesma racional do set_focus/trio de rolagem acima: operam sobre
//     texto/classes/propriedades, não geometria).
// ---------------------------------------------------------------------------

bool UiLayer::set_text(const char* id, const char* text) const {
  if (!ready()) return false;
  return impl_->engine.set_text(id, text);
}

bool UiLayer::add_class(const char* id, const char* cls) const {
  if (!ready()) return false;
  return impl_->engine.add_class(id, cls);
}

bool UiLayer::remove_class(const char* id, const char* cls) const {
  if (!ready()) return false;
  return impl_->engine.remove_class(id, cls);
}

bool UiLayer::set_property(const char* id, const char* prop, const char* value) const {
  if (!ready()) return false;
  return impl_->engine.set_property(id, prop, value);
}

bool UiLayer::load_font_face(const FontFaceDesc& desc) {
  if (!ready()) return false;
  // EN: Straight passthrough (UI-FONTFACE) -- no coordinate translation applicable (font
  //     registration carries no geometry), no document-loaded requirement either (unlike the
  //     id-keyed DOM methods above -- see this method's own doc-comment in ui_layer.hpp).
  // PT: Repasse direto (UI-FONTFACE) -- sem tradução de coordenada aplicável (registro de
  //     fonte não carrega geometria), sem exigência de documento carregado também (diferente
  //     dos métodos de DOM indexados por id acima -- ver o próprio doc-comment deste método em
  //     ui_layer.hpp).
  return impl_->engine.load_font_face(desc);
}

bool UiLayer::get_number(const char* key, double& out) const {
  if (!ready()) return false;
  return impl_->engine.get_number(key, out);
}

bool UiLayer::get_string(const char* key, std::string& out) const {
  if (!ready()) return false;
  return impl_->engine.get_string(key, out);
}

bool UiLayer::get_bool(const char* key, bool& out) const {
  if (!ready()) return false;
  return impl_->engine.get_bool(key, out);
}

void UiLayer::process_event(const UiEvent& ev) {
  // EN: Guard: drop events when the layer is not ready or context is gone. Same shape as
  //     Engine::process_event's own guard below -- kept here too so the Resize branch (which
  //     never reaches the Engine) is still guarded.
  // PT: Guard: descarta eventos quando a camada não está pronta ou contexto sumiu. Mesmo
  //     formato do guard do próprio Engine::process_event abaixo -- mantido aqui também para
  //     que o branch de Resize (que nunca chega ao Engine) continue guardado.
  if (!ready()) return;

  // EN: Resize is the ONE case that stays here (A1, framework-2D refactor,
  //     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md section 2.1): it mutates
  //     facade-owned letterbox state (impl_->w/h/gl_offset_y), not just an RmlUi call, so it
  //     cannot move to the shared Engine::process_event. Every other case (None/MouseMove/
  //     MouseButton/Key/Text/MouseWheel) is now IDENTICAL behaviour, just delegated -- see
  //     Engine::process_event's doc-comment in engine.hpp for the moved switch and the two
  //     AUD-TEC-2 non-finite guards it still enforces.
  // PT: Resize é o ÚNICO caso que permanece aqui (refactor A1, framework-2D,
  //     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md seção 2.1): muta estado de
  //     letterbox de posse da fachada (impl_->w/h/gl_offset_y), não é só uma chamada ao RmlUi,
  //     então não pode mudar para o Engine::process_event compartilhado. Todo outro caso
  //     (None/MouseMove/MouseButton/Key/Text/MouseWheel) tem comportamento IDÊNTICO agora, só
  //     delegado -- ver o doc-comment de Engine::process_event em engine.hpp para o switch
  //     movido e os dois guards AUD-TEC-2 de não-finito que ele ainda enforça.
  if (ev.type == UiEvent::Type::Resize) {
    // EN: RMLX-0/F3 -- the "context is gone" half of this guard used to be a local
    //     `Rml::Context* c = impl_->engine.context(); if (!c) return;`; replaced by
    //     Engine::ok(), the same guard shape every OTHER Engine caller in this file already
    //     uses (see ready()/impl_->engine.set_viewport() below), so this branch no longer names
    //     an Rml:: type at all.
    // PT: RMLX-0/F3 -- a metade "contexto sumiu" deste guard costumava ser um
    //     `Rml::Context* c = impl_->engine.context(); if (!c) return;` local; substituída por
    //     Engine::ok(), o MESMO formato de guard que todo OUTRO chamador do Engine neste arquivo
    //     já usa (ver ready()/impl_->engine.set_viewport() abaixo), então este branch deixa de
    //     nomear qualquer tipo Rml:: por completo.
    if (!impl_->engine.ok()) return;
    // EN: Logical viewport resize — update cached dimensions and notify RmlUi.
    //     The next render_compose() uses the updated impl_->w/h via set_viewport().
    // PT: Redimensionamento lógico do viewport — atualiza dimensões cacheadas e notifica RmlUi.
    //     O próximo render_compose() usa o impl_->w/h atualizado via set_viewport().

    // EN: Guard: zero/negative dimensions are invalid for the layout engine — skip silently.
    //     Deliberately kept as its OWN local check, not folded into Engine::set_viewport's
    //     internal guard below: that one ALSO warns via glintfx::log_warn on rejection
    //     (QW-GUARDLOG), which would change this branch's long-standing silent-skip behaviour
    //     for a resize event -- this local guard keeps that untouched by never letting an
    //     invalid width/height reach Engine::set_viewport in the first place.
    // PT: Guard: dimensões zero/negativas são inválidas para o motor de layout — ignorar
    //     silenciosamente. Deliberadamente mantido como checagem LOCAL própria, não dobrado no
    //     guard interno de Engine::set_viewport abaixo: aquele TAMBÉM avisa via
    //     glintfx::log_warn ao rejeitar (QW-GUARDLOG), o que mudaria o comportamento de
    //     ignorar-em-silêncio, de longa data, deste branch para um evento de resize -- este
    //     guard local mantém isso intocado ao nunca deixar uma largura/altura inválida chegar a
    //     Engine::set_viewport, de saída.
    if (ev.width <= 0 || ev.height <= 0) return;

    impl_->w = ev.width;
    impl_->h = ev.height;
    impl_->engine.set_viewport(ev.width, ev.height);
    // EN: F3 (v0.2.5) -- keep gl_offset_y consistent with the new h when in letterbox mode
    //     (x/target_h stay whatever the last explicit set_viewport(x,y,w,h,target_h) set).
    //     Legacy mode stays at offset 0 -- unchanged behaviour, gl_offset_x untouched (does
    //     not depend on h).
    // PT: F3 (v0.2.5) -- mantém gl_offset_y consistente com o novo h em modo letterbox
    //     (x/target_h continuam o que o último set_viewport(x,y,w,h,target_h) explícito
    //     setou). Modo legado permanece em offset 0 -- comportamento inalterado,
    //     gl_offset_x intocado (não depende de h).
    if (impl_->letterbox_mode)
      impl_->gl_offset_y = impl_->target_h - impl_->y - impl_->h;
    return;
  }

  // EN: Every other event type: delegate to the shared Engine route, passing the letterbox
  //     origin as the offset (impl_->x, impl_->y) -- IDENTICAL to what this method's own
  //     switch used to subtract inline for MouseMove (F3, v0.2.5).
  // PT: Todo outro tipo de evento: delega à rota compartilhada do Engine, passando a origem
  //     de letterbox como offset (impl_->x, impl_->y) -- IDÊNTICO ao que o switch deste
  //     método subtraía inline para o MouseMove (F3, v0.2.5).
  impl_->engine.process_event(ev, impl_->x, impl_->y);
}

// EN: `FRAMEGRAB-EMBED` -- see this method's own doc-comment (ui_layer.hpp, right before
//     `private:`) for the full contract. Pure delegation to the SAME Engine::capture_frame
//     readback App::capture_frame (app.cpp) uses -- this method does NOT render (the host's
//     own render() call, made before this one, is what put the desired content into FBO 0)
//     and does NOT swap (the host owns that too). The only UiLayer-specific work here is
//     picking the CORRECT region to read: the current viewport's own GL-native offset/size
//     (impl_->gl_offset_x/y/w/h), the SAME rectangle render() -> Engine::render_compose just
//     composited into -- pre-computed once by set_viewport() (see its own doc-comment) and
//     reused here unchanged, exactly like render() itself reuses it every frame.
// PT: `FRAMEGRAB-EMBED` -- ver o próprio doc-comment deste método (ui_layer.hpp, logo antes
//     de `private:`) pro contrato completo. Delegação pura pro MESMO readback
//     Engine::capture_frame que App::capture_frame (app.cpp) usa -- este método NÃO
//     renderiza (a própria chamada render() do host, feita antes desta, é o que pôs o
//     conteúdo desejado no FBO 0) e NÃO faz swap (o host também é dono disso). O único
//     trabalho específico do UiLayer aqui é escolher a região CORRETA a ler: o próprio
//     offset/tamanho nativo-GL do viewport corrente (impl_->gl_offset_x/y/w/h), o MESMO
//     retângulo que render() -> Engine::render_compose acabou de compor -- pré-calculado uma
//     vez por set_viewport() (ver o próprio doc-comment dele) e reusado aqui sem mudança,
//     exatamente como o próprio render() o reusa a cada frame.
//
// EN: CAPTURE-NOTHROW (W22, 2026-07-30) -- noexcept added, audited instruction-by-instruction
//     (see this method's own doc-comment, ui_layer.hpp, for the full reasoning): every statement
//     below is either a plain bool/field read-or-copy, the delegation to the now noexcept-safe
//     Engine::capture_frame() (engine.cpp, this same slice), or a unique_ptr move-assignment --
//     none of those can throw.
// PT: CAPTURE-NOTHROW (W22, 2026-07-30) -- noexcept somado, auditado instrução-por-instrução
//     (ver o próprio doc-comment deste método, ui_layer.hpp, pro racional completo): toda
//     instrução abaixo é ou uma leitura/cópia pura de bool/campo, a delegação pro agora
//     noexcept-seguro Engine::capture_frame() (engine.cpp, esta mesma fatia), ou uma atribuição-
//     por-move de unique_ptr -- nenhuma delas pode lançar.
UiLayer::CapturedFrame UiLayer::capture_frame() const noexcept {
  if (!ready()) return CapturedFrame{};
  CapturedFramePixels px =
      impl_->engine.capture_frame(impl_->gl_offset_x, impl_->gl_offset_y, impl_->w, impl_->h);

  CapturedFrame out;
  out.ok = px.ok;
  out.width = px.width;
  out.height = px.height;
  out.byte_count = px.byte_count;
  out.pixels = std::move(px.pixels);
  return out;
}

} // namespace glintfx
