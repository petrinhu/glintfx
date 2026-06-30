# ADR-0008 — Embed / guest mode / Modo embutido (guest)

- **Status:** Accepted (2026-06-30)
- **Deciders:** petrus (líder), software-architect
- **Tags:** architecture, api, additive, two-way-door
- **Relates to:** [[0006-layered-hybrid-architecture]] (Layer 1 = the C++23 deliverable).

## Context / Contexto

**EN:** glintfx v1 fuses two responsibilities into one `App` facade: **(1)** ownership of the window, GL context and frame loop (GLFW), and **(2)** the UI + GL3 effects engine (RmlUi + `RenderInterface_GL3`). For a **UI-only app**, fusing both is exactly the drop-in win. But the first real consumer is a **game** — GusWorld / GusEngine — which already owns its window, GL context and loop via **SDL3** (a deliberate choice in their ADR-008 for AAA gamepad, audio and a console path) and renders its **own scene** (sprites, tilemap, damage floaters) through its own GL3 2D renderer (`Render2dGl3`), composing the RmlUi HUD on top (their ADR-009). Such a host needs **only responsibility (2)**, attached to **its own** GL context. v1 cannot serve this, for two concrete collisions identified in the integration verdict:

- **G1 — window ownership:** `App` always creates and owns a GLFW window; it cannot attach to a context the host already made current. A GLFW context cannot be shared with an SDL3-owned window — the context must be created by whoever owns the window.
- **G2 — closed frame:** `App::render()` is sealed as `begin_frame → UI render → end_frame → swap`, and clears/swaps the buffer. A game needs to draw its scene **first**, have the UI compose **on top without clearing**, and own the **single swap** itself.
- **G3 — input source:** `App` feeds RmlUi from GLFW events; an SDL3 host feeds RmlUi from SDL events (its own platform adapter). The event source must be injectable.

**PT:** O glintfx v1 funde duas responsabilidades numa única fachada `App`: **(1)** posse da janela, do contexto GL e do loop de frame (GLFW), e **(2)** o motor de UI + efeitos GL3 (RmlUi + `RenderInterface_GL3`). Para um **app só-de-UI**, fundir as duas é justamente o ganho drop-in. Mas o primeiro consumidor real é um **jogo** — GusWorld / GusEngine — que já possui a própria janela, contexto GL e loop via **SDL3** (escolha deliberada no ADR-008 deles por gamepad AAA, áudio e caminho pra console) e renderiza a **própria cena** (sprites, tilemap, floaters de dano) por um renderer GL3 2D próprio (`Render2dGl3`), compondo o HUD RmlUi por cima (ADR-009 deles). Esse host precisa de **só a responsabilidade (2)**, anexada ao **seu próprio** contexto GL. A v1 não atende a isso, por duas colisões concretas apontadas no veredito de integração:

- **G1 — posse da janela:** o `App` sempre cria e possui uma janela GLFW; não consegue anexar a um contexto que o host já tornou corrente. Um contexto GLFW não pode ser compartilhado com uma janela do SDL3 — o contexto tem de ser criado por quem possui a janela.
- **G2 — frame fechado:** o `App::render()` é selado como `begin_frame → render UI → end_frame → swap`, e limpa/troca o buffer. Um jogo precisa desenhar a cena **primeiro**, ter a UI compondo **por cima sem limpar**, e fazer ele mesmo o **swap único**.
- **G3 — fonte de input:** o `App` alimenta o RmlUi com eventos GLFW; um host SDL3 alimenta o RmlUi com eventos SDL (seu próprio adapter de plataforma). A fonte de eventos precisa ser injetável.

## Decision / Decisão

**EN:** Add an **embed / guest mode** as a **purely additive** capability. The standalone `App` (window owner) stays **intact** for UI-only apps; we factor responsibility **(2)** out of it into a new facade — a **`UiLayer`** — that a host drives over **its own** GL context:

- **(a) Attach, don't create:** `UiLayer` is constructed against a **GL context the host already created and made current** (via SDL3, GLFW, X11, … — glintfx doesn't care). It loads GL function pointers if not yet loaded (gl3w is idempotent) and initialises RmlUi + `RenderInterface_GL3`. It **never** opens a window.
- **(b) Compose-only frame:** the per-frame call emits **only** the UI geometry — **no `glClear`, no buffer swap**. The host clears at the start of its frame, draws its scene, calls `UiLayer::render()`, and performs the **single** swap. This generalizes RmlUi's `BeginFrame`/`EndFrame` so the render layer composites on top of the host's framebuffer (premultiplied-alpha) without erasing it.
- **(c) Injected events:** the host pushes input/resize/clock through a thin adapter (a `SystemInterface`-shaped seam). glintfx ships an optional GLFW adapter; an SDL3 host supplies its own (it already has `RmlUi_Platform_SDL`). The UI engine never assumes GLFW.
- **(d) GL state contract:** with two owners on one context, `UiLayer::render()` **saves and restores** the GL state it touches (viewport, blend, scissor, bound program/VAO/textures, depth/stencil test) so the host's renderer sees the context as it left it — and documents the small set of invariants it relies on the host to honour.

A first sketch of the public surface (illustrative — **not** an implementation commitment; names/signatures to be settled in the build task):

```cpp
namespace glintfx {

// Effects + UI engine attached to a host-owned, host-current GL context.
// Move-only. Does NOT own a window and never swaps buffers.
class UiLayer {
public:
  struct Config {
    int  logical_width  = 1280;   // authoring resolution (dp); scaled to pixels
    int  logical_height = 720;
    bool load_gl = true;          // load GL pointers via gl3w (idempotent); false if host did it
  };

  explicit UiLayer(Config cfg = {});   // requires a current GL context
  ~UiLayer();
  UiLayer(UiLayer&&) noexcept;
  UiLayer& operator=(UiLayer&&) noexcept;
  UiLayer(const UiLayer&) = delete;
  UiLayer& operator=(const UiLayer&) = delete;

  bool ok() const noexcept;            // attach + RmlUi init succeeded

  void load(const char* rml_path);     // load + show an .rml document
  void set_pixel_size(int w, int h);   // real target pixels (resize)

  // Injected input: the host translates its native events and pushes them in.
  // (SDL3 hosts already have an RmlUi platform adapter for this.)
  void push_mouse_move(float x, float y);
  void push_mouse_button(int button, bool down);
  void push_key(int rml_key, bool down);
  void push_text(const char* utf8);

  void update();                       // advance UI context (layout/animations)

  // Compose-only: emits UI geometry over the CURRENT framebuffer.
  // Saves/restores GL state. Does NOT clear. Does NOT swap. The host swaps.
  void render();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
```

**PT:** Adicionar um **modo embutido (guest)** como capacidade **puramente aditiva**. O `App` standalone (dono da janela) permanece **intacto** para apps só-de-UI; fatoramos a responsabilidade **(2)** dele para uma nova fachada — uma **`UiLayer`** — que um host dirige sobre o **seu próprio** contexto GL:

- **(a) Anexar, não criar:** a `UiLayer` é construída contra um **contexto GL que o host já criou e tornou corrente** (via SDL3, GLFW, X11, … — o glintfx não se importa). Carrega os ponteiros de função GL se ainda não carregados (o gl3w é idempotente) e inicializa o RmlUi + `RenderInterface_GL3`. **Nunca** abre uma janela.
- **(b) Frame compose-only:** a chamada por-frame emite **só** a geometria da UI — **sem `glClear`, sem swap de buffer**. O host limpa no início do seu frame, desenha a cena, chama `UiLayer::render()`, e faz o swap **único**. Isso generaliza o `BeginFrame`/`EndFrame` do RmlUi para o render layer compor por cima do framebuffer do host (premultiplied-alpha) sem apagá-lo.
- **(c) Eventos injetados:** o host empurra input/resize/clock por um adapter fino (uma costura no formato de `SystemInterface`). O glintfx oferece um adapter GLFW opcional; um host SDL3 fornece o seu (ele já tem o `RmlUi_Platform_SDL`). O motor de UI nunca assume GLFW.
- **(d) Contrato de estado GL:** com dois donos num único contexto, o `UiLayer::render()` **salva e restaura** o estado GL que toca (viewport, blend, scissor, program/VAO/texturas vinculados, teste de depth/stencil) para que o renderer do host veja o contexto como o deixou — e documenta o pequeno conjunto de invariantes que confia ao host respeitar.

(O esboço da API pública acima é ilustrativo — **não** é compromisso de implementação; nomes/assinaturas serão fechados na tarefa de build.)

## Consequences / Consequências

**EN:** Enables GusWorld's **model B** (glintfx as UI overlay on the context the game owns): GusWorld keeps SDL3 + `Render2dGl3` + its composition order, and **retires** its hand-vendored `rmlui_hud` + `RmlUi_Renderer_GL3` + `gl3_loader`, swapping them for `glintfx::UiLayer`. Their POCO core/domain (~1013 tests) are untouched. **It widens glintfx's mandate** from "drop-in for UI-only apps" to "embeddable game UI" — a deliberate, accepted scope growth (the first consumer justifies it). **New risk:** shared GL state between two owners on one context — mitigated by (d)'s save/restore contract and an explicit invariant doc. The standalone `App` is unaffected (it can be re-expressed as a thin host that owns a GLFW window and drives a `UiLayer`, keeping one engine path). **v2 (the Atomic-Design component library) must treat embed mode as a first-class consumer:** its data-driven RML/RCSS components must work identically whether driven by `App` or by a host's `UiLayer`, otherwise the components are useless to games (the very audience v2 targets). This is a **two-way door**: embed mode is additive surface; if it proves wrong it can be deprecated without touching the standalone path.

**PT:** Habilita o **modelo B** do GusWorld (glintfx como overlay de UI no contexto que o jogo possui): o GusWorld mantém SDL3 + `Render2dGl3` + sua ordem de composição, e **aposenta** seu `rmlui_hud` + `RmlUi_Renderer_GL3` + `gl3_loader` vendorizados à mão, trocando-os por `glintfx::UiLayer`. O core/domain POCO deles (~1013 testes) ficam intactos. **Alarga o mandato do glintfx** de "drop-in para apps só-de-UI" para "UI de jogo embutível" — crescimento de escopo deliberado e aceito (o primeiro consumidor o justifica). **Risco novo:** estado GL compartilhado entre dois donos num contexto — mitigado pelo contrato de save/restore do item (d) e por um doc explícito de invariantes. O `App` standalone não é afetado (pode ser reexpresso como um host fino que possui uma janela GLFW e dirige uma `UiLayer`, mantendo um único caminho de engine). **A v2 (a component library de Atomic Design) deve tratar o embed mode como consumidor de 1ª classe:** seus componentes data-driven RML/RCSS têm de funcionar idênticos quer dirigidos pelo `App`, quer pela `UiLayer` de um host, senão os componentes são inúteis para jogos (justamente o público da v2). É uma **porta de mão dupla**: o embed mode é superfície aditiva; se se provar errado, pode ser depreciado sem tocar no caminho standalone.

## F1 implementation constraints / Limitações da implementação F1

**EN:** Discovered during the F1 build (confirmed by reading the pinned RmlUi GL3 backend source):

- **FBO target is always FBO 0.** `RenderInterface_GL3::EndFrame()` unconditionally calls
  `glBindFramebuffer(GL_FRAMEBUFFER, 0)` and blits the postprocessed UI onto FBO 0 (the window
  backbuffer). There is no API to redirect output to a caller-supplied FBO. Consequence: the host
  **must use FBO 0** as its scene render target; `UiLayer::render()` composites the UI on top of
  whatever is already in FBO 0 via premultiplied-alpha blend, without clearing it. A custom FBO
  bound by the host before `render()` is **not** used as the composition target (the
  `GlStateGuard` will restore its binding, but rendering output still goes to FBO 0).

- **Viewport origin is hardcoded to (0, 0).** `begin_frame_compose()` calls
  `glViewport(0, 0, w, h)`. Sub-region compositing (a UI panel at a pixel offset within the window)
  is not supported in F1. The host must present a full-window render target.

Both constraints are consistent with the primary consumer (GusWorld / SDL3): the host renders
its game scene to the window (FBO 0), calls `UiLayer::render()`, and the UI is overlaid on the
full window before the SDL3 swap.

**PT:** Descobertas durante o build da F1 (confirmadas pela leitura do source do backend GL3 do RmlUi pinado):

- **Alvo de FBO é sempre FBO 0.** `RenderInterface_GL3::EndFrame()` chama incondicionalmente
  `glBindFramebuffer(GL_FRAMEBUFFER, 0)` e blitta a UI pós-processada no FBO 0 (backbuffer da
  janela). Não existe API para redirecionar output para um FBO fornecido pelo chamador.
  Consequência: o host **deve usar FBO 0** como alvo de render da cena; `UiLayer::render()`
  compõe a UI por cima do que já está no FBO 0 via blend premultiplied-alpha, sem limpar.
  Um FBO customizado do host ligado antes de `render()` **não** é usado como alvo de composição
  (o `GlStateGuard` restaura o binding, mas o output do render vai para FBO 0).

- **Origem do viewport é hardcoded em (0, 0).** `begin_frame_compose()` chama
  `glViewport(0, 0, w, h)`. Composição em sub-região (painel UI em offset de pixel dentro da
  janela) não é suportada na F1. O host deve apresentar um render target de janela inteira.

Ambas as restrições são consistentes com o consumidor primário (GusWorld / SDL3): o host
renderiza a cena na janela (FBO 0), chama `UiLayer::render()`, e a UI é sobreposta sobre a
janela inteira antes do swap do SDL3.

## Options considered / Opções consideradas

- **Additive embed mode (`UiLayer`) (chosen / escolhida)** — keeps the UI-only `App` intact; serves games over their own context; two-way door.
- glintfx owns the window via SDL3 backend / glintfx dono da janela com backend SDL3 — would let `App` host the game, but inverts frame control and still needs a scene hook; bigger, against the grain. Rejected.
- Game re-pivots to GLFW so `App` can own it / Jogo re-pivota pra GLFW — regressa o ADR-008 do GusWorld (gamepad/áudio/console). Rejected.
- Partial adoption: glintfx only on full-screen non-scene UI / Adoção parcial — leaves the battle cockpit (the actual pain) out. Rejected.

Cross-ref: [[0006-layered-hybrid-architecture]]. Integration verdict gaps **G1–G3**.
