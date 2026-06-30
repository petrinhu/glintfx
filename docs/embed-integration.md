# Embed mode integration guide / Guia de integração do embed mode

**EN:** Integration contract for hosts that own their window and GL context and embed glintfx as a UI overlay through `glintfx::UiLayer` (the embed/guest mode of [ADR-0008](adr/0008-embed-guest-mode.md)). First consumer: GusWorld (SDL3 + its own `Render2dGl3`). This document is the authoritative reference for what `UiLayer` guarantees and what the host must honour. Line references point at the source as of glintfx v0.2.1.

**PT:** Contrato de integração para hosts que possuem a própria janela e contexto GL e embutem o glintfx como overlay de UI via `glintfx::UiLayer` (o embed/guest mode do [ADR-0008](adr/0008-embed-guest-mode.md)). Primeiro consumidor: GusWorld (SDL3 + o `Render2dGl3` próprio). Este documento é a referência autoritativa do que o `UiLayer` garante e do que o host deve honrar. As referências de linha apontam para o código na v0.2.1 do glintfx.

## 0. Frame lifecycle / Ciclo de frame

**EN:** Per frame, the host stays in control. The order is fixed:

**PT:** Por frame, o host permanece no controle. A ordem é fixa:

```
1. host: make the GL context current / tornar o contexto GL corrente
2. host: glClear + draw its scene into the window backbuffer (FBO 0)
         glClear + desenhar a cena no backbuffer da janela (FBO 0)
3. ui.process_event(ev) ...   // inject host input / injetar input do host
4. ui.update();               // advance the UI / avançar a UI
5. ui.render();               // compose UI over FBO 0, no clear, no swap
                              // compor a UI sobre o FBO 0, sem clear, sem swap
6. host: swap buffers / trocar buffers
```

**EN:** `UiLayer` never clears the framebuffer and never swaps buffers. The host owns both.

**PT:** O `UiLayer` nunca limpa o framebuffer e nunca troca buffers. O host é dono dos dois.

## 1. Coordinates and DPI / Coordenadas e DPI

**EN:**
- `set_viewport(w, h)` expects the **real backbuffer pixels**. The same `(w, h)` drives two coupled things, 1:1: the RmlUi context dimensions (the layout coordinate space) via `Engine::set_viewport` -> `context->SetDimensions` (`glintfx/src/engine.cpp:44-49`, cached in `ui_layer.cpp:98-103`), and the GL viewport via `render_compose` -> `begin_frame_compose` -> `glViewport(0,0,w,h)` + `renderer.SetViewport(w,h)` (`glintfx/src/render_gl3.cpp:114-115`).
- Rendering is **1:1**: one RCSS pixel equals one GL pixel. There is **no dp to px scaling** in this release. Confirmed: no `SetDensityIndependentPixelRatio` / dp_ratio anywhere in `glintfx/src`.
- `UiLayerConfig{logical_width = 1280, logical_height = 720}` (`glintfx/include/glintfx/ui_layer.hpp:19-23`) is only the **initial** context size passed at attach (`ui_layer.cpp:67-81`). `set_viewport` overrides it. The field name "logical" is a slight misnomer in this release: there is no logical space distinct from the physical one.
- Composition always targets the **window backbuffer (FBO 0)** with viewport origin hardcoded at `(0,0)` (`render_gl3.cpp:114`, and the note in `ui_layer.hpp:64-79`). A custom host FBO bound before `render()` is **not** used as the composition target. Sub region compositing is not supported.
- **Authoring at a fixed logical size (for example 960x540):** there is no built in upscaling. Either author the RML/RCSS at the real backbuffer size and pass real pixels to `set_viewport`, or pass `960x540` and accept that both the context and the GL viewport become `960x540` anchored at `(0,0)` (not scaled to fill the screen). Crisp integer upscaling of a fixed logical canvas is a known gap (see section 6).

**PT:**
- `set_viewport(w, h)` espera os **pixels reais do backbuffer**. Os mesmos `(w, h)` alimentam duas coisas acopladas, 1:1: a dimensão do contexto RmlUi (o espaço de layout) via `Engine::set_viewport` -> `context->SetDimensions` (`glintfx/src/engine.cpp:44-49`, cacheado em `ui_layer.cpp:98-103`), e o viewport GL via `render_compose` -> `begin_frame_compose` -> `glViewport(0,0,w,h)` + `renderer.SetViewport(w,h)` (`glintfx/src/render_gl3.cpp:114-115`).
- O render é **1:1**: um pixel de RCSS equivale a um pixel de GL. **Não há scaling dp para px** nesta versão. Confirmado: não existe `SetDensityIndependentPixelRatio` / dp_ratio em `glintfx/src`.
- `UiLayerConfig{logical_width = 1280, logical_height = 720}` (`glintfx/include/glintfx/ui_layer.hpp:19-23`) é só a dimensão **inicial** do contexto, passada no attach (`ui_layer.cpp:67-81`). `set_viewport` a sobrescreve. O nome "logical" do campo é um leve misnomer nesta versão: não há espaço lógico distinto do físico.
- A composição mira sempre o **backbuffer da janela (FBO 0)** com origem do viewport fixa em `(0,0)` (`render_gl3.cpp:114`, e a nota em `ui_layer.hpp:64-79`). Um FBO custom do host ligado antes de `render()` **não** é usado como alvo. Composição em sub região não é suportada.
- **Autoria num tamanho lógico fixo (por exemplo 960x540):** não há upscaling embutido. Ou autore a RML/RCSS no tamanho real do backbuffer e passe pixels reais a `set_viewport`, ou passe `960x540` e aceite que o contexto e o viewport GL viram `960x540` ancorados em `(0,0)` (não escalado para preencher a tela). O upscaling inteiro nítido de um canvas lógico fixo é um gap conhecido (ver seção 6).

## 2. GL state contract / Contrato de estado GL

**EN:** `glintfx::GlStateGuard` (`glintfx/src/gl_state.hpp:20-63`) is taken around every `render()` (`engine.cpp:77-82`). It snapshots and restores exactly the following:

**PT:** O `glintfx::GlStateGuard` (`glintfx/src/gl_state.hpp:20-63`) é tomado em volta de cada `render()` (`engine.cpp:77-82`). Ele salva e restaura exatamente o seguinte:

| State / Estado | save | restore |
| :--- | :--- | :--- |
| Viewport (`GL_VIEWPORT`) | :21 | :48 |
| Scissor box (`GL_SCISSOR_BOX`) + enable (`GL_SCISSOR_TEST`) | :23, :27 | :49, :51 |
| Blend enable (`GL_BLEND`) | :22 | :50 |
| Blend func separate (SRC/DST RGB + ALPHA) | :28-31 | :55-56 |
| Blend equation separate (RGB + ALPHA) | :36-37 | :57 |
| Cull face enable (`GL_CULL_FACE`) | :24 | :52 |
| Depth test enable (`GL_DEPTH_TEST`) | :25 | :53 |
| Stencil test enable (`GL_STENCIL_TEST`) | :26 | :54 |
| Current program (`GL_CURRENT_PROGRAM`) | :38 | :58 |
| Vertex array binding / VAO (`GL_VERTEX_ARRAY_BINDING`) | :39 | :59 |
| Active texture unit (`GL_ACTIVE_TEXTURE`) | :40 | :60 |
| 2D texture on the active unit (`GL_TEXTURE_BINDING_2D`) | :41 | :61 |
| Draw FBO binding (`GL_DRAW_FRAMEBUFFER_BINDING`) | :42 | :47 |
| Colour write mask (`GL_COLOR_WRITEMASK`) | :43 | :62 |

**EN: Host invariants (what is NOT restored, so the host must honour it):**
1. **Re bind the VBO.** `GL_ARRAY_BUFFER_BINDING` is not saved or restored. The host must bind its own vertex buffer before drawing after `render()`. The element buffer (`GL_ELEMENT_ARRAY_BUFFER`) is part of VAO state and is restored via the VAO binding, but the array buffer bind point is global and is left dirty.
2. **Depth mask and depth func.** Only the depth test enable bit is restored. If the host relies on a specific `glDepthMask` or `glDepthFunc`, it must reset them.
3. **Textures on other units.** Only the 2D texture bound on the active unit observed is restored. Do not assume bindings on other texture units survive; re bind per draw.
4. **Out of scope state.** Pixel store / unpack alignment, sRGB framebuffer enable, polygon mode, primitive restart, and anything not in the table above are not restored.
5. **Host owns clear and swap.** The host clears and draws its scene into FBO 0 and calls `render()` afterwards, before the swap. `render()` always composes onto FBO 0; a custom host FBO is not the target (`ui_layer.hpp:64-79`). Viewport origin is `(0,0)`.
6. **Current context.** The host must make its GL context current before constructing the `UiLayer` and before every `render()`.

**PT: Invariantes do host (o que NÃO é restaurado, então o host deve honrar):**
1. **Re vincular o VBO.** `GL_ARRAY_BUFFER_BINDING` não é salvo nem restaurado. O host deve vincular o próprio vertex buffer antes de desenhar depois de `render()`. O element buffer (`GL_ELEMENT_ARRAY_BUFFER`) é estado de VAO e é restaurado via o binding de VAO, mas o bind point de array buffer é global e fica sujo.
2. **Depth mask e depth func.** Só o enable do depth test é restaurado. Se o host depende de um `glDepthMask` ou `glDepthFunc` específico, deve re setar.
3. **Texturas em outras unidades.** Só a textura 2D ligada na unidade ativa observada é restaurada. Não assuma que bindings em outras unidades de textura sobrevivem; re vincule por draw.
4. **Estado fora de escopo.** Pixel store / unpack alignment, sRGB framebuffer enable, polygon mode, primitive restart e qualquer coisa fora da tabela acima não são restaurados.
5. **Host é dono do clear e do swap.** O host limpa e desenha a cena no FBO 0 e chama `render()` depois, antes do swap. O `render()` sempre compõe no FBO 0; um FBO custom do host não é o alvo (`ui_layer.hpp:64-79`). Origem do viewport é `(0,0)`.
6. **Contexto corrente.** O host deve tornar o contexto GL corrente antes de construir o `UiLayer` e antes de cada `render()`.

## 3. GL loader / Loader GL

**EN:**
- glintfx uses **gl3w** via `RMLUI_GL3_CUSTOM_LOADER=<GL/gl3w.h>` (`glintfx/CMakeLists.txt:101`) and links `gl3w` (`:117`). MSAA is disabled (`RMLUI_NUM_MSAA_SAMPLES=0`, `:110`).
- `UiLayerConfig.load_gl` (`ui_layer.hpp:22`, default `true`): when `true`, the constructor calls `gl3wInit()` against the host current context, loading glintfx's own gl3w function pointer table (`ui_layer.cpp:77-79`). When `false`, glintfx assumes gl3w was already initialised in this process.
- **Coexistence with a host loader (for example GusWorld's glad based loader): supported.** gl3w and glad keep separate function pointer tables, both resolved from the same context. They do not conflict, but glintfx's gl3w table must be initialised for glintfx to work.
- **Recommendation:** a host with its own loader keeps `load_gl = true` (the default). glintfx loads its own gl3w; the host keeps its loader for its own renderer. Set `load_gl = false` only if the host also uses gl3w and already called `gl3wInit()`. Precondition: the GL context must be current before constructing the `UiLayer` (`gl3wInit` needs a current context).

**PT:**
- O glintfx usa **gl3w** via `RMLUI_GL3_CUSTOM_LOADER=<GL/gl3w.h>` (`glintfx/CMakeLists.txt:101`) e linka `gl3w` (`:117`). MSAA desligado (`RMLUI_NUM_MSAA_SAMPLES=0`, `:110`).
- `UiLayerConfig.load_gl` (`ui_layer.hpp:22`, default `true`): quando `true`, o construtor chama `gl3wInit()` contra o contexto corrente do host, carregando a tabela de ponteiros gl3w do próprio glintfx (`ui_layer.cpp:77-79`). Quando `false`, o glintfx assume que o gl3w já foi inicializado no processo.
- **Coexistência com um loader do host (por exemplo o loader baseado em glad do GusWorld): suportada.** gl3w e glad mantêm tabelas de ponteiros separadas, ambas resolvidas do mesmo contexto. Não conflitam, mas a tabela gl3w do glintfx precisa ser inicializada para o glintfx funcionar.
- **Recomendação:** um host com loader próprio mantém `load_gl = true` (o default). O glintfx carrega o próprio gl3w; o host mantém o loader dele para o renderer dele. Use `load_gl = false` só se o host também usar gl3w e já tiver chamado `gl3wInit()`. Pré condição: o contexto GL deve estar corrente antes de construir o `UiLayer` (o `gl3wInit` exige contexto corrente).

## 4. Asset resolution / Resolução de assets

**EN:**
- `load(rml_path)` forwards to `LoadDocument(rml_path)` (`glintfx/src/bootstrap.cpp:63`). RmlUi resolves relative URLs (the `<link href>` in the RML, and `@font-face src` / `image()` / decorator paths in the RCSS) **relative to the file that references them** (the `.rml` for `<link>`, the `.rcss` for fonts and images), **not** relative to the process working directory.
- glintfx provides **no base URL override** in this release. (The `set_asset_base_url` that some hosts had elsewhere does not exist here.)
- **How to point at fonts and sprites:** call `load("<dir>/cockpit.rml")` and place `cockpit.rcss`, the font file, and sprites at paths the RML/RCSS reference relative to their own location. For example `@font-face { src: "fonts/PixelOperatorMono.ttf"; }` resolves to `<dir>/fonts/...`. The bundled showcase confirms the pattern.
- **Cautions:** use a stable working directory or consistent relative paths; absolute paths in RmlUi URLs may lose the leading slash during canonicalisation, so prefer relative paths anchored at the document. An absolute asset root would need a base URL override (see section 6).

**PT:**
- `load(rml_path)` encaminha para `LoadDocument(rml_path)` (`glintfx/src/bootstrap.cpp:63`). O RmlUi resolve URLs relativas (o `<link href>` no RML, e `@font-face src` / `image()` / decorator no RCSS) **relativo ao arquivo que as referencia** (o `.rml` para o `<link>`, o `.rcss` para fontes e imagens), **não** relativo ao diretório de trabalho do processo.
- O glintfx **não oferece override de base URL** nesta versão. (O `set_asset_base_url` que alguns hosts tinham em outro lugar não existe aqui.)
- **Como apontar fontes e sprites:** chame `load("<dir>/cockpit.rml")` e coloque `cockpit.rcss`, o arquivo de fonte e os sprites em caminhos que a RML/RCSS referencia relativo à própria localização. Por exemplo `@font-face { src: "fonts/PixelOperatorMono.ttf"; }` resolve para `<dir>/fonts/...`. O showcase embarcado confirma o padrão.
- **Cautelas:** use um diretório de trabalho estável ou caminhos relativos consistentes; caminhos absolutos em URLs do RmlUi podem perder a barra inicial na canonicalização, então prefira caminhos relativos ancorados no documento. Uma raiz de asset absoluta exigiria um override de base URL (ver seção 6).

## 5. Focus navigation / Navegação por foco

**EN:**
- `process_event` with `UiEvent::Type::Key` forwards `ProcessKeyDown/Up` to the RmlUi context (`glintfx/src/ui_layer.cpp:143-154`), mapping `Key::Up/Down/Left/Right` to the arrow keys, `Enter` to Return, `Escape` to Escape, and `Tab` to Tab (`ui_layer.cpp:31-44`). A host maps its gamepad and keyboard to `glintfx::Key` and injects through `process_event`.
- **Proven today:** events reach the context without a crash (this is the event plumbing).
- **Not yet movement:** actual focus movement between elements requires the **document** to declare focusable elements (`tabindex`) and, for directional arrow navigation, the RCSS `nav` property. The standalone embed path does not ship focusable components, so the events arrive but there is nothing to navigate.
- **For a host menu (for example a verb menu driven by WASD, arrows, or gamepad):** map the input to `glintfx::Key` and inject it (ready today), and author the menu RML with `tabindex` plus `nav`, or use a focus first menu component. `Tab` cycling works out of the box for elements with `tabindex`; arrow navigation requires the `nav` property.

**PT:**
- `process_event` com `UiEvent::Type::Key` encaminha `ProcessKeyDown/Up` ao contexto RmlUi (`glintfx/src/ui_layer.cpp:143-154`), mapeando `Key::Up/Down/Left/Right` para as setas, `Enter` para Return, `Escape` para Escape e `Tab` para Tab (`ui_layer.cpp:31-44`). O host mapeia o gamepad e o teclado para `glintfx::Key` e injeta via `process_event`.
- **Provado hoje:** os eventos chegam ao contexto sem crash (este é o encaminhamento do evento).
- **Ainda não o movimento:** o movimento de foco real entre elementos exige o **documento** declarar elementos focáveis (`tabindex`) e, para a navegação direcional por setas, a propriedade RCSS `nav`. O caminho embed por si só não traz componentes focáveis, então os eventos chegam mas não há o que navegar.
- **Para um menu do host (por exemplo um menu de verbos por WASD, setas ou gamepad):** mapeie o input para `glintfx::Key` e injete (pronto hoje), e autore o RML do menu com `tabindex` mais `nav`, ou use um componente de menu focus first. O ciclo por `Tab` funciona de imediato para elementos com `tabindex`; a navegação por setas exige a propriedade `nav`.

## 6. Known limitations and gaps / Limitações conhecidas e gaps

**EN:** Candidate features for a later release:
1. **Logical to physical scaling (dp_ratio).** Expose RmlUi's density independent pixel ratio so a fixed logical canvas (for example 960x540) can scale crisply to a larger backbuffer (for example 1080p). Today rendering is strictly 1:1.
2. **Custom host FBO target plus configurable viewport origin.** Today `render()` always composes onto FBO 0 at origin `(0,0)`. Compositing into a host owned offscreen FBO, or into a sub region, is not supported.
3. **Asset base URL override.** Allow an absolute asset root to be set so documents can reference assets independently of their own path.

**PT:** Features candidatas para uma versão futura:
1. **Escala lógico para físico (dp_ratio).** Expor o density independent pixel ratio do RmlUi para que um canvas lógico fixo (por exemplo 960x540) escale com nitidez para um backbuffer maior (por exemplo 1080p). Hoje o render é estritamente 1:1.
2. **Alvo de FBO custom do host mais origem de viewport configurável.** Hoje o `render()` sempre compõe no FBO 0 na origem `(0,0)`. Compor num FBO offscreen do host, ou numa sub região, não é suportado.
3. **Override de base URL de assets.** Permitir definir uma raiz de asset absoluta para que documentos referenciem assets independente do próprio caminho.

## See also / Veja também

- [ADR-0008](adr/0008-embed-guest-mode.md): embed/guest mode decision, including the GL state save and restore clause (d). / decisão do embed/guest mode, incluindo a cláusula (d) de save e restore de estado GL.
- [`README.md`](../README.md): library overview and the standalone `glintfx::App` path. / visão geral da lib e o caminho standalone `glintfx::App`.
- [`docs/effects.md`](effects.md): RCSS effect syntax used inside embedded documents. / sintaxe de efeitos RCSS usada dentro de documentos embutidos.
