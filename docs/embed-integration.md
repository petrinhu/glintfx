# Embed mode integration guide / Guia de integração do embed mode

**EN:** Integration contract for hosts that own their window and GL context and embed glintfx as a UI overlay through `glintfx::UiLayer` (the embed/guest mode of [ADR-0008](adr/0008-embed-guest-mode.md)). First consumer: GusWorld (SDL3 + its own `Render2dGl3`). This document is the authoritative reference for what `UiLayer` guarantees and what the host must honour. Line references are approximate; verify against the current source.

**PT:** Contrato de integração para hosts que possuem a própria janela e contexto GL e embutem o glintfx como overlay de UI via `glintfx::UiLayer` (o embed/guest mode do [ADR-0008](adr/0008-embed-guest-mode.md)). Primeiro consumidor: GusWorld (SDL3 + o `Render2dGl3` próprio). Este documento é a referência autoritativa do que o `UiLayer` garante e do que o host deve honrar. As referências de linha são aproximadas; verifique contra o código-fonte atual.

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

**EN:** When using the letterbox `set_viewport(x, y, w, h, target_h)` overload (F3, v0.2.5), the host must clear/redraw its scene across the **FULL window** at step 2 (not just the sub-region) before calling `render()` -- `render()` itself never clears, and the sub-viewport it composites into no longer covers the whole backbuffer. See section 10 for the full contract.

**PT:** Ao usar a sobrecarga letterbox `set_viewport(x, y, w, h, target_h)` (F3, v0.2.5), o host deve limpar/redesenhar a cena na **janela INTEIRA** no passo 2 (não só na sub-região) antes de chamar `render()` -- o `render()` em si nunca limpa, e a sub-viewport na qual ele compõe não cobre mais o backbuffer inteiro. Ver a seção 10 para o contrato completo.

## 1. Coordinates and DPI / Coordenadas e DPI

**EN:**
- `set_viewport(w, h)` expects the **real backbuffer pixels**. The same `(w, h)` drives two coupled things, 1:1: the RmlUi context dimensions (the layout coordinate space) via `Engine::set_viewport` -> `context->SetDimensions` (`glintfx/src/engine.cpp:44-49`, cached in `ui_layer.cpp:98-103`), and the GL viewport via `render_compose` -> `begin_frame_compose` -> `glViewport(0,0,w,h)` + `renderer.SetViewport(w,h)` (`glintfx/src/render_gl3.cpp:114-115`).
- `UiLayerConfig{logical_width = 1280, logical_height = 720}` (`glintfx/include/glintfx/ui_layer.hpp`) is only the **initial** context size passed at attach. `set_viewport` overrides it.
- Composition always targets the **window backbuffer (FBO 0)**. The viewport **origin** within FBO 0 is configurable since v0.2.5 via `UiLayer::set_viewport(x, y, w, h, target_h)` -- see section 10. A custom host FBO bound before `render()` is still **not** used as the composition target.
- **dp_ratio (density-independent pixel ratio).** `UiLayerConfig::dp_ratio` (default `1.0f`) and `UiLayer::set_dp_ratio(float)` expose `Rml::Context::SetDensityIndependentPixelRatio`. When set, the RCSS `dp` unit scales: `1dp = dp_ratio physical px`. The `px` RCSS unit is always one physical pixel and is NOT affected by dp_ratio.
  - **Recommended pattern:** author RCSS in `dp` units at a fixed logical canvas (e.g. 960×540), call `set_viewport(1920, 1080)` (physical backbuffer), and `set_dp_ratio(1920.0f / 960.0f)` = `2.0f`. RmlUi scales the layout to fill the physical backbuffer without any manual RCSS edits.
  - `App` has parity: `AppConfig::dp_ratio` and `App::set_dp_ratio(float)`.
  - Verified: `dp_ratio_sanity` test renders a 100dp×100dp white box; scale factor at ratio=2.0 vs ratio=1.0 is exactly **4.0×** (200×200 px vs 100×100 px).
- **`get_element_box(id)` (F2, v0.2.5)** returns `ElementBox{found, x, y, w, h}` -- the element's **border-box** (`Rml::BoxArea::Border`: includes border, excludes margin) in the **same physical-pixel, top-left-origin, y-down space as `set_viewport(w, h)`**. If the RCSS geometry is authored in `dp` units, the returned `x/y/w/h` are already the **scaled physical-pixel result** (post-`dp_ratio` multiplication) -- callers never need to multiply by `dp_ratio` themselves. `found=false` (all fields zero) when the id does not exist in the currently loaded document, or no document is loaded yet. Parity: `App::get_element_box` (same signature, same units -- `App` owns the whole window so there is no additional sub-viewport offset to account for). Header: `glintfx/include/glintfx/element_box.hpp`. Verified: `element_box_sanity`/`app_element_box_smoke` tests.

**PT:**
- `set_viewport(w, h)` espera os **pixels reais do backbuffer**. Os mesmos `(w, h)` alimentam duas coisas acopladas, 1:1: a dimensão do contexto RmlUi via `Engine::set_viewport` -> `context->SetDimensions` e o viewport GL via `render_compose` -> `begin_frame_compose`.
- `UiLayerConfig{logical_width = 1280, logical_height = 720}` é só a dimensão **inicial** do contexto. `set_viewport` a sobrescreve.
- A composição mira sempre o **backbuffer da janela (FBO 0)**. A **origem** do viewport dentro do FBO 0 é configurável desde a v0.2.5 via `UiLayer::set_viewport(x, y, w, h, target_h)` -- ver seção 10. Um FBO custom do host ligado antes de `render()` ainda **não** é usado como alvo de composição.
- **dp_ratio (density-independent pixel ratio).** `UiLayerConfig::dp_ratio` (padrão `1.0f`) e `UiLayer::set_dp_ratio(float)` expõem `Rml::Context::SetDensityIndependentPixelRatio`. Quando definido, a unidade RCSS `dp` escala: `1dp = dp_ratio px físicos`. A unidade `px` do RCSS é sempre um pixel físico e NÃO é afetada pelo dp_ratio.
  - **Padrão recomendado:** autore o RCSS em unidades `dp` num canvas lógico fixo (ex.: 960×540), chame `set_viewport(1920, 1080)` (backbuffer físico) e `set_dp_ratio(1920.0f / 960.0f)` = `2.0f`. O RmlUi escala o layout para preencher o backbuffer físico sem nenhuma edição de RCSS.
  - `App` tem paridade: `AppConfig::dp_ratio` e `App::set_dp_ratio(float)`.
  - Verificado: teste `dp_ratio_sanity` renderiza box branco 100dp×100dp; fator de escala em ratio=2.0 vs ratio=1.0 é exatamente **4.0×** (200×200 px vs 100×100 px).
- **`get_element_box(id)` (F2, v0.2.5)** retorna `ElementBox{found, x, y, w, h}` -- o **border-box** do elemento (`Rml::BoxArea::Border`: inclui borda, exclui margem) no **mesmo espaço de pixels físicos, origem superior-esquerda, y pra baixo, de `set_viewport(w, h)`**. Se a geometria RCSS for autorada em unidades `dp`, o `x/y/w/h` retornado já é o **resultado em pixels físicos escalado** (pós-multiplicação por `dp_ratio`) -- chamadores nunca precisam multiplicar por `dp_ratio` eles mesmos. `found=false` (todos os campos zero) quando o id não existe no documento carregado atualmente, ou nenhum documento carregado ainda. Paridade: `App::get_element_box` (mesma assinatura, mesmas unidades -- o `App` é dono da janela inteira, então não há offset adicional de sub-viewport a considerar). Header: `glintfx/include/glintfx/element_box.hpp`. Verificado: testes `element_box_sanity`/`app_element_box_smoke`.

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
5. **Host owns clear and swap.** The host clears and draws its scene into FBO 0 and calls `render()` afterwards, before the swap. `render()` always composes onto FBO 0; a custom host FBO is not the target (`ui_layer.hpp:64-79`). Viewport origin within FBO 0 is `(0,0)` by default, configurable since v0.2.5 (section 10).
6. **Current context.** The host must make its GL context current before constructing the `UiLayer` and before every `render()`.

**PT: Invariantes do host (o que NÃO é restaurado, então o host deve honrar):**
1. **Re vincular o VBO.** `GL_ARRAY_BUFFER_BINDING` não é salvo nem restaurado. O host deve vincular o próprio vertex buffer antes de desenhar depois de `render()`. O element buffer (`GL_ELEMENT_ARRAY_BUFFER`) é estado de VAO e é restaurado via o binding de VAO, mas o bind point de array buffer é global e fica sujo.
2. **Depth mask e depth func.** Só o enable do depth test é restaurado. Se o host depende de um `glDepthMask` ou `glDepthFunc` específico, deve re setar.
3. **Texturas em outras unidades.** Só a textura 2D ligada na unidade ativa observada é restaurada. Não assuma que bindings em outras unidades de textura sobrevivem; re vincule por draw.
4. **Estado fora de escopo.** Pixel store / unpack alignment, sRGB framebuffer enable, polygon mode, primitive restart e qualquer coisa fora da tabela acima não são restaurados.
5. **Host é dono do clear e do swap.** O host limpa e desenha a cena no FBO 0 e chama `render()` depois, antes do swap. O `render()` sempre compõe no FBO 0; um FBO custom do host não é o alvo (`ui_layer.hpp:64-79`). A origem do viewport dentro do FBO 0 é `(0,0)` por padrão, configurável desde a v0.2.5 (seção 10).
6. **Contexto corrente.** O host deve tornar o contexto GL corrente antes de construir o `UiLayer` e antes de cada `render()`.

## 3. GL loader / Loader GL

**EN:**
- glintfx uses its own **GL 3.3 core-profile loader** (`glintfx/src/gl_loader.h`/`.c`, generated from the public Khronos `gl.xml` registry, L1.14-GLLOADER; replaces the previously vendored gl3w) via `RMLUI_GL3_CUSTOM_LOADER=<gl_loader.h>` (`glintfx/CMakeLists.txt:187`) and links the `glloader` INTERFACE target (`:203`). MSAA is disabled (`RMLUI_NUM_MSAA_SAMPLES=0`).
- `UiLayerConfig.load_gl` (`ui_layer.hpp:22`, default `true`): when `true`, the constructor calls `glx_gl_load()` against the host current context, loading glintfx's own GL function pointer table (`ui_layer.cpp`). When `false`, glintfx assumes the loader was already initialised in this process.
- **Coexistence with a host loader (for example GusWorld's glad based loader): supported.** glintfx's loader and glad keep separate function pointer tables, both resolved from the same context. They do not conflict, but glintfx's own table must be initialised for glintfx to work.
- **Recommendation:** a host with its own loader keeps `load_gl = true` (the default). glintfx loads its own GL function pointers; the host keeps its loader for its own renderer. Set `load_gl = false` only if the host also uses glintfx's loader and already called `glx_gl_load()`. Precondition: the GL context must be current before constructing the `UiLayer` (`glx_gl_load` needs a current context).

**PT:**
- O glintfx usa seu **próprio loader GL 3.3 core profile** (`glintfx/src/gl_loader.h`/`.c`, gerado a partir do registro público Khronos `gl.xml`, L1.14-GLLOADER; substitui o gl3w antes vendorizado) via `RMLUI_GL3_CUSTOM_LOADER=<gl_loader.h>` (`glintfx/CMakeLists.txt:187`) e linka o target INTERFACE `glloader` (`:203`). MSAA desligado (`RMLUI_NUM_MSAA_SAMPLES=0`).
- `UiLayerConfig.load_gl` (`ui_layer.hpp:22`, default `true`): quando `true`, o construtor chama `glx_gl_load()` contra o contexto corrente do host, carregando a tabela de ponteiros GL do próprio glintfx (`ui_layer.cpp`). Quando `false`, o glintfx assume que o loader já foi inicializado no processo.
- **Coexistência com um loader do host (por exemplo o loader baseado em glad do GusWorld): suportada.** O loader do glintfx e o glad mantêm tabelas de ponteiros separadas, ambas resolvidas do mesmo contexto. Não conflitam, mas a tabela do glintfx precisa ser inicializada para o glintfx funcionar.
- **Recomendação:** um host com loader próprio mantém `load_gl = true` (o default). O glintfx carrega os próprios ponteiros de função GL; o host mantém o loader dele para o renderer dele. Use `load_gl = false` só se o host também usar o loader do glintfx e já tiver chamado `glx_gl_load()`. Pré condição: o contexto GL deve estar corrente antes de construir o `UiLayer` (o `glx_gl_load` exige contexto corrente).

## 4. Asset resolution / Resolução de assets

**EN:**
- `load(rml_path)` forwards to `LoadDocument(rml_path)` (`glintfx/src/bootstrap.cpp`). RmlUi resolves relative URLs (the `<link href>` in the RML, and `@font-face src` / `image()` / decorator paths in the RCSS) **relative to the document URL** (which is the path passed to `load()`), **not** relative to the process working directory.
- **`set_asset_base_url(const char*)`.** glintfx installs a custom `Rml::FileInterface` (`glintfx/src/base_url_file_interface.hpp`) before `Rml::Initialise()`. When `set_asset_base_url(url)` is called, all subsequent relative paths passed to `Open()` are resolved as `url/path` instead of being looked up from the process CWD. Absolute paths (starting with `/`) bypass the prefix. A fallback to the raw path is attempted if the prefixed open fails.
  - **Call before `load()`** so the document itself also resolves via the base URL.
  - Pass `nullptr` or `""` to clear.
  - `App::set_asset_base_url(const char*)` provides parity with `UiLayer`.
  - Verified: `base_url_sanity` test loads `embed_scene.rml` (not present in the CWD) successfully after `set_asset_base_url("base_url_assets")`. The rendered frame passes all three pixel statistics.
- **How to point at fonts and sprites (traditional pattern):** call `load("<dir>/panel.rml")` and place `panel.rcss`, the font file, and sprites at paths the RML/RCSS reference relative to their own location. For example `@font-face { src: "fonts/PixelOperatorMono.ttf"; }` resolves to `<dir>/fonts/...`. This pattern works without any base URL.
- **Cautions:** absolute paths in RmlUi URLs may lose the leading slash during canonicalisation, so prefer relative paths. The base URL override is most useful for absolute asset roots or when the process CWD cannot be controlled.

**PT:**
- `load(rml_path)` encaminha para `LoadDocument(rml_path)` (`glintfx/src/bootstrap.cpp`). O RmlUi resolve URLs relativas **relativo à URL do documento** (o path passado a `load()`), **não** relativo ao diretório de trabalho do processo.
- **`set_asset_base_url(const char*)`.** O glintfx instala um `Rml::FileInterface` customizado (`glintfx/src/base_url_file_interface.hpp`) antes de `Rml::Initialise()`. Quando `set_asset_base_url(url)` é chamado, todos os caminhos relativos subsequentes passados a `Open()` são resolvidos como `url/path` em vez de serem buscados no CWD do processo. Caminhos absolutos (começando com `/`) ignoram o prefixo. Um fallback para o path bruto é tentado se o open prefixado falhar.
  - **Chame antes de `load()`** para que o documento em si também resolva via o base URL.
  - Passe `nullptr` ou `""` para limpar.
  - `App::set_asset_base_url(const char*)` tem paridade com `UiLayer`.
  - Verificado: teste `base_url_sanity` carrega `embed_scene.rml` (ausente no CWD) com sucesso após `set_asset_base_url("base_url_assets")`. O frame renderizado passa as três estatísticas de pixel.
- **Como apontar fontes e sprites (padrão tradicional):** chame `load("<dir>/panel.rml")` e coloque `panel.rcss`, o arquivo de fonte e os sprites em caminhos que a RML/RCSS referencia relativo à própria localização. Por exemplo `@font-face { src: "fonts/PixelOperatorMono.ttf"; }` resolve para `<dir>/fonts/...`. Este padrão funciona sem nenhum base URL.
- **Cautelas:** caminhos absolutos em URLs do RmlUi podem perder a barra inicial na canonicalização; prefira caminhos relativos. O override de base URL é mais útil para raízes de asset absolutas ou quando o CWD do processo não pode ser controlado.

## 5. Focus navigation / Navegação por foco

**EN:**
- `process_event` with `UiEvent::Type::Key` forwards `ProcessKeyDown/Up` to the RmlUi context (`glintfx/src/ui_layer.cpp:143-154`), mapping `Key::Up/Down/Left/Right` to the arrow keys, `Enter` to Return, `Escape` to Escape, and `Tab` to Tab (`ui_layer.cpp:31-44`). A host maps its gamepad and keyboard to `glintfx::Key` and injects through `process_event`.
- **Proven today:** events reach the context without a crash (this is the event plumbing).
- **Not yet movement:** actual focus movement between elements requires the **document** to declare focusable elements (`tabindex`) and, for directional arrow navigation, the RCSS `nav` property. The standalone embed path does not ship focusable components, so the events arrive but there is nothing to navigate.
- **For a host menu (for example a menu driven by WASD, arrow keys, or a gamepad):** map the input to `glintfx::Key` and inject it (ready today), and author the menu RML with `tabindex` plus `nav`, or use a focus first menu component. `Tab` cycling works out of the box for elements with `tabindex`; arrow navigation requires the `nav` property.

**PT:**
- `process_event` com `UiEvent::Type::Key` encaminha `ProcessKeyDown/Up` ao contexto RmlUi (`glintfx/src/ui_layer.cpp:143-154`), mapeando `Key::Up/Down/Left/Right` para as setas, `Enter` para Return, `Escape` para Escape e `Tab` para Tab (`ui_layer.cpp:31-44`). O host mapeia o gamepad e o teclado para `glintfx::Key` e injeta via `process_event`.
- **Provado hoje:** os eventos chegam ao contexto sem crash (este é o encaminhamento do evento).
- **Ainda não o movimento:** o movimento de foco real entre elementos exige o **documento** declarar elementos focáveis (`tabindex`) e, para a navegação direcional por setas, a propriedade RCSS `nav`. O caminho embed por si só não traz componentes focáveis, então os eventos chegam mas não há o que navegar.
- **Para um menu do host (por exemplo um menu por WASD, setas ou gamepad):** mapeie o input para `glintfx::Key` e injete (pronto hoje), e autore o RML do menu com `tabindex` mais `nav`, ou use um componente de menu focus first. O ciclo por `Tab` funciona de imediato para elementos com `tabindex`; a navegação por setas exige a propriedade `nav`.

**EN:** glintfx does not expose a `set_focus(id)` method today: focus is glintfx-owned, and `Tab`/arrow keys move it on their own through `tabindex`/`nav` as described above. Hosts whose MODEL is the owner of selection (rather than the RmlUi focus ring) should drive the highlight through **data-binding** (a bound class or attribute via the data model, section 6), not through `:focus`. This is the pattern used by GusWorld. `set_focus(id)` is a roadmap item, tracked as `GAP-4` in the INBOX of `TODO.md`.

**PT:** O glintfx não expõe um método `set_focus(id)` hoje: o foco é de posse do glintfx, e `Tab`/setas o movem sozinhos pelo `tabindex`/`nav` descritos acima. Hosts cujo MODELO é o dono da seleção (em vez do anel de foco do RmlUi) devem dirigir o destaque por **data-binding** (classe ou atributo ligado via o data model, seção 6), não por `:focus`. Este é o padrão usado pelo GusWorld. `set_focus(id)` é item de roadmap, rastreado como `GAP-4` na INBOX do `TODO.md`.

## 6. Data model / Modelo de dados

**EN:** glintfx exposes RmlUi's data-binding system through a thin, type-safe C surface (9 methods total) with no engine-specific types in the public header. The same API is available on both `UiLayer` and `App`.

**PT:** O glintfx expõe o sistema de data-binding do RmlUi por uma superfície C fina e type-safe (9 métodos no total) sem tipos internos do engine no header público. A mesma API está disponível em `UiLayer` e `App`.

### Mandatory call order / Ordem obrigatória de chamada

**EN:** The order is fixed and enforced by the engine (out-of-order calls return `false`):

**PT:** A ordem é fixa e enforçada pelo engine (chamadas fora de ordem retornam `false`):

```
create_data_model(name)                    // 1. declare the model
bind_number / bind_string / bind_bool / bind_list(key, ...)  // 2. register variables
load(rml_path)                             // 3. compile document (views bind here)
set_number / set_string / set_bool / set_list(key, ...)      // 4. update each frame
```

**EN:** `bind_*` called **after** `load()` returns `false`: RmlUi compiles the data-binding views at document load time and variable addresses must already be registered. The engine enforces the constraint and logs the violation.

**PT:** `bind_*` chamado **apos** `load()` retorna `false`: o RmlUi compila as views de data-binding no momento do carregamento do documento e os endereços de variável precisam estar registrados antes desse ponto. O engine enforça a restrição e registra a violação.

### API surface / Superfície da API

**EN:** All 9 methods are on `UiLayer` ([`glintfx/include/glintfx/ui_layer.hpp`](../glintfx/include/glintfx/ui_layer.hpp)) and `App` ([`glintfx/include/glintfx/app.hpp`](../glintfx/include/glintfx/app.hpp)):

**PT:** Todos os 9 métodos estão em `UiLayer` ([`glintfx/include/glintfx/ui_layer.hpp`](../glintfx/include/glintfx/ui_layer.hpp)) e `App` ([`glintfx/include/glintfx/app.hpp`](../glintfx/include/glintfx/app.hpp)):

| Method / Método | Returns | Description / Descrição |
| :--- | :--- | :--- |
| `create_data_model(name)` | `bool` | Declare model; call before `bind_*` and before `load()`. / Declara model; chamar antes de `bind_*` e de `load()`. |
| `bind_number(key, initial=0.0)` | `bool` | Register a `double` variable. / Registra variável `double`. |
| `bind_string(key, initial="")` | `bool` | Register a string variable. / Registra variável string. |
| `bind_bool(key, initial=false)` | `bool` | Register a `bool` variable. / Registra variável `bool`. |
| `bind_list(key)` | `bool` | Register a string-list; iterable in RML via `data-for`. / Registra lista de strings; iterável no RML via `data-for`. |
| `set_number(key, value)` | `void` | Update a bound number. / Atualiza número ligado. |
| `set_string(key, value)` | `void` | Update a bound string. / Atualiza string ligada. |
| `set_bool(key, value)` | `void` | Update a bound bool. / Atualiza bool ligado. |
| `set_list(key, items, count)` | `void` | Replace list contents atomically. / Substitui conteúdo da lista atomicamente. |

**EN:** Memory ownership: the bound variable slots are owned by the glintfx engine (`DataBinder` holds one `std::unique_ptr` per key in a `std::map` for stable addresses). The consumer passes values by copy and must not manage or hold raw pointers into cell memory.

**PT:** Posse de memória: os slots de variável ligados são de posse do engine (`DataBinder` guarda um `std::unique_ptr` por chave em `std::map` para endereços estáveis). O consumidor passa valores por cópia e não deve gerenciar nem guardar ponteiros brutos para a memória das células.

### Overlay panel example / Exemplo de painel overlay

**EN:** A generic overlay panel with a numeric status value, two label strings, and a scrolling log list. The `data-for` attribute iterates the list; `{{line}}` is the per-item alias.

**PT:** Um painel overlay genérico com um valor numérico de status, duas strings de rótulo e um log rolante. O atributo `data-for` itera a lista; `{{line}}` é o alias por item.

```cpp
// C++ -- create and bind before load()
ui.create_data_model("panel");
ui.bind_number("status", 100.0);
ui.bind_string("label",  "");
ui.bind_string("detail", "");
ui.bind_list  ("log");

ui.load("panel.rml");   // views compile here

// per-frame update
ui.set_number("status", state.status);
ui.set_string("label",  state.label);
ui.set_string("detail", state.detail);
const char* lines[] = { log[0].c_str(), log[1].c_str(), log[2].c_str() };
ui.set_list("log", lines, 3);
```

```html
<!-- panel.rml excerpt -->
<body data-model="panel">
  <p>Status {{status}}</p>
  <p>{{label}} -- {{detail}}</p>
  <p data-for="line : log">{{line}}</p>
</body>
```

**EN:** The same pattern works with `App` (standalone mode). See the `data_model_smoke`, `data_model_scalar`, and `data_model_list` tests in `glintfx/tests/` for runnable examples.

**PT:** O mesmo padrão funciona com `App` (modo standalone). Ver os testes `data_model_smoke`, `data_model_scalar` e `data_model_list` em `glintfx/tests/` para exemplos executáveis.

---

## 7. Texture formats / Formatos de textura

**EN:** glintfx supports **PNG, JPEG (JPG), and TGA** for images referenced in RCSS (`image()`, `decorator:`, `@sprite-sheet`). Decoding is handled by **stb_image** (vendored at `glintfx/third_party/stb/stb_image.h`) via an override of `Rml::RenderInterface::LoadTexture`. The same backend serves both `App` and `UiLayer`.

**PT:** O glintfx suporta **PNG, JPEG (JPG) e TGA** para imagens referenciadas em RCSS (`image()`, `decorator:`, `@sprite-sheet`). A decodificação e feita pelo **stb_image** (vendorizado em `glintfx/third_party/stb/stb_image.h`) via override de `Rml::RenderInterface::LoadTexture`. O mesmo backend serve `App` e `UiLayer`.

### Alpha premultiplication / Premultiplicacao de alpha

**EN:** After decoding, glintfx **premultiplies the alpha channel in-place** before uploading to the GPU: `out.rgb = in.rgb * in.a / 255; out.a = in.a`. This is required for correct compositing: the GL3 renderer blends with `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` (premultiplied-alpha blend), consistent with the opaque-backbuffer contract in section 0. Without premultiplication, a PNG with straight alpha produces a bright halo around transparent regions.

**PT:** Apos decodificar, o glintfx **premultiplica o canal alpha in-place** antes do upload para a GPU: `out.rgb = in.rgb * in.a / 255; out.a = in.a`. Isso e obrigatorio para composicao correta: o renderer GL3 mistura com `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` (blend premultiplied-alpha), consistente com o contrato de backbuffer opaco da secao 0. Sem premultiplicacao, um PNG com alpha straight produz um halo claro ao redor das regioes transparentes.

### Format recommendations / Recomendacoes de formato

**EN:**
- **PNG 32-bit (RGBA):** recommended for UI frames, portraits, icons, and any asset with transparency. The premultiplication step handles the alpha correctly.
- **JPEG (JPG):** suitable for opaque background images (no alpha channel; premultiplication is a no-op). Lossy -- avoid for crisp UI line art.
- **TGA:** legacy fallback; still supported. glintfx falls back to the upstream RmlUi TGA loader if stb_image decoding fails, so existing TGA assets do not regress.

**PT:**
- **PNG 32-bit (RGBA):** recomendado para molduras de UI, retratos, icones e qualquer asset com transparencia. A etapa de premultiplicacao trata o alpha corretamente.
- **JPEG (JPG):** adequado para imagens de fundo opacas (sem canal alpha; premultiplicacao e no-op). Com perda -- evitar para arte com linhas nítidas de UI.
- **TGA:** fallback legado; ainda suportado. O glintfx cai no loader TGA do upstream RmlUi se a decodificacao do stb_image falhar, entao assets TGA existentes nao regridem.

**EN:** Place image files alongside the RML/RCSS that references them. RmlUi resolves texture paths relative to the referencing document (section 4). The `set_asset_base_url` override also applies to texture paths.

**PT:** Coloque os arquivos de imagem ao lado do RML/RCSS que os referencia. O RmlUi resolve os caminhos de textura relativos ao documento que os referencia (secao 4). O override de `set_asset_base_url` tambem se aplica aos caminhos de textura.

---

## 8. Known limitations and gaps / Limitações conhecidas e gaps

**EN:** Candidate features for a later release:
1. ~~**Logical to physical scaling (dp_ratio).**~~ **Resolved in v0.2.2.** `UiLayerConfig::dp_ratio`, `UiLayer::set_dp_ratio(float)`, `AppConfig::dp_ratio`, and `App::set_dp_ratio(float)` are now available. See section 1 for the full contract.
2. ~~**Configurable viewport origin.**~~ **Resolved in v0.2.5.** `UiLayer::set_viewport(x, y, w, h, target_h)` places the composited UI within a sub-region of the host's window. See section 10.
3. **Custom host FBO target.** `render()` still always composes onto FBO 0; compositing into a host-owned offscreen FBO is not supported.
4. ~~**Asset base URL override.**~~ **Resolved in v0.2.2.** `UiLayer::set_asset_base_url(const char*)` and `App::set_asset_base_url(const char*)` are now available. See section 4 for the full contract.

**PT:** Features candidatas para uma versão futura:
1. ~~**Escala lógico para físico (dp_ratio).**~~ **Resolvido na v0.2.2.** `UiLayerConfig::dp_ratio`, `UiLayer::set_dp_ratio(float)`, `AppConfig::dp_ratio` e `App::set_dp_ratio(float)` estão disponíveis. Ver seção 1 para o contrato completo.
2. ~~**Origem de viewport configurável.**~~ **Resolvido na v0.2.5.** `UiLayer::set_viewport(x, y, w, h, target_h)` posiciona a UI composta numa sub-região da janela do host. Ver seção 10.
3. **Alvo de FBO custom do host.** O `render()` ainda sempre compõe no FBO 0; compor num FBO offscreen de posse do host não é suportado.
4. ~~**Override de base URL de assets.**~~ **Resolvido na v0.2.2.** `UiLayer::set_asset_base_url(const char*)` e `App::set_asset_base_url(const char*)` estão disponíveis. Ver seção 4 para o contrato completo.

## 9. UA stylesheet (display: block defaults) / Stylesheet UA (defaults de display: block)

**EN:** RmlUi's built-in `display` default is `inline` for every element (unlike HTML,
where `div`/`p`/`h1`.../`section`/... are block by default). glintfx merges a small
embedded User-Agent stylesheet into every document loaded by `load()` (both `App` and
`UiLayer` -- shared code path in `Bootstrap::load()`):
```
div, p, h1, h2, h3, h4, h5, h6, ul, ol, li,
section, article, header, footer, nav, main { display: block; }
```
It is applied as a **low-specificity base**: any author rule of equal or higher
specificity in your own RCSS overrides it (this is standard CSS cascade behaviour, not
a special case). You do not need to `@import` anything or opt in -- it is always on.
Elements outside this list (e.g. `span`, custom tags) still default to `inline`, matching
RmlUi's own behaviour; add your own `display` rule for those if needed.

**PT:** O padrão embutido do RmlUi para `display` é `inline` para todo elemento
(diferente do HTML, onde `div`/`p`/`h1`.../`section`/... são block por padrão). O
glintfx mescla uma pequena stylesheet User-Agent embutida em todo documento carregado
por `load()` (tanto `App` quanto `UiLayer` -- caminho de código compartilhado em
`Bootstrap::load()`):
```
div, p, h1, h2, h3, h4, h5, h6, ul, ol, li,
section, article, header, footer, nav, main { display: block; }
```
É aplicada como **base de baixa especificidade**: qualquer regra do autor com
especificidade igual ou maior no seu próprio RCSS a sobrepõe (comportamento padrão de
cascata CSS, não um caso especial). Não é necessário `@import` nem opt-in -- está sempre
ativa. Elementos fora desta lista (ex.: `span`, tags customizadas) continuam com default
`inline`, igual ao próprio RmlUi; adicione sua própria regra `display` se precisar.

## 10. Click callback, element geometry, and the window-space coordinate contract (v0.2.5) / Callback de clique, geometria de elemento e o contrato de coordenadas espaço-janela (v0.2.5)

**EN:** Three additions close the "host mirrors RCSS geometry by hand" gap:

- **`set_click_callback(std::function<void(const char* element_id)>)`** (`UiLayer` + `App`, parity): fires on every "click" (bubble phase, listener attached to the document root by `load()`). Reports the id of the click TARGET, or the nearest ANCESTOR with a non-empty id if the target itself has none, or `""` if no element in the ancestor chain has an id. No ordering constraint versus `load()` -- can be set before or after. Verified by `click_callback_sanity` and `app_click_callback_smoke`.
- **`get_element_box(const char* id) -> ElementBox { bool found; float x, y, w, h; }`** (`UiLayer` + `App`, parity): border-box geometry of an element by id. Units, dp_ratio interaction, and the `found=false` contract are documented in section 1 -- this section only adds the coordinate-SPACE guarantee below.
- **`UiLayer::set_viewport(int x, int y, int w, int h, int target_h)`** (`UiLayer`-only, F3): places the composited UI within a sub-region of the host's window (letterbox). The legacy `set_viewport(w, h)` is unchanged and equivalent to `(0, 0, w, h, h)`. See section 0 for the host's per-frame clear obligation under letterbox mode.

**The coordinate contract (read this before using any of the three):** every coordinate that crosses the public API boundary -- `UiEvent`'s mouse `x`/`y`, `get_element_box()`'s return value, and `set_viewport`'s `x`/`y` -- is **window/render-target physical pixels, top-left origin, y-down**. This is the SAME convention regardless of whether a viewport offset is active. `UiLayer` performs ALL translation to/from its own offset-free content space internally: it subtracts `(x, y)` from mouse coordinates before forwarding to RmlUi, and adds `(x, y)` to `get_element_box()`'s raw content-space result before returning it. **The host never needs to convert coordinates for input or geometry queries.** The ONE place OpenGL's native bottom-up viewport convention leaks through is `set_viewport`'s `target_h` parameter (the host's own window height) -- needed only because `glViewport` is unavoidably bottom-left-origin at the final composite step (`gl_offset_y = target_h - y - h`); see the ADR-0008 "F3 implementation update" section for the exact formula. `App` never has an offset (it owns its whole window), so `App::get_element_box()` needs no translation and `App` does not gain `set_viewport`.

**Why F1/F2/F3 are documented together:** they share one coordinate system by design, and a sign error in any of the three translation points (`process_event`'s `MouseMove`, `get_element_box`, `set_viewport`'s formula) is exactly the class of silent desync bug that `set_click_callback` was built to eliminate on the host side -- reintroduced inside glintfx instead. The integration test `viewport_origin_sanity` exercises all three together as one oracle-checked scenario (click-through-offset, geometry-through-offset, and the actual GL pixel placement) rather than in isolation.

**PT:** Três adições fecham a lacuna "o host espelha geometria do RCSS à mão":

- **`set_click_callback(std::function<void(const char* element_id)>)`** (`UiLayer` + `App`, paridade): dispara em todo "click" (fase bubble, listener anexado à raiz do documento por `load()`). Reporta o id do ALVO do clique, ou o ANCESTRAL mais próximo com id não-vazio se o próprio alvo não tiver, ou `""` se nenhum elemento na cadeia de ancestrais tiver id. Sem restrição de ordem vs. `load()` -- pode ser setado antes ou depois. Verificado por `click_callback_sanity` e `app_click_callback_smoke`.
- **`get_element_box(const char* id) -> ElementBox { bool found; float x, y, w, h; }`** (`UiLayer` + `App`, paridade): geometria border-box de um elemento por id. Unidades, interação com dp_ratio e o contrato `found=false` estão documentados na seção 1 -- esta seção só acrescenta a garantia de ESPAÇO de coordenadas abaixo.
- **`UiLayer::set_viewport(int x, int y, int w, int h, int target_h)`** (`UiLayer`-only, F3): posiciona a UI composta numa sub-região da janela do host (letterbox). O `set_viewport(w, h)` legado é inalterado e equivalente a `(0, 0, w, h, h)`. Ver seção 0 para a obrigação de clear por-frame do host em modo letterbox.

**O contrato de coordenadas (leia antes de usar qualquer uma das três):** toda coordenada que cruza a fronteira pública da API -- `x`/`y` de mouse do `UiEvent`, o retorno de `get_element_box()`, e `x`/`y` de `set_viewport` -- é **pixels físicos do render-target/janela, origem superior-esquerda, y pra baixo**. É a MESMA convenção independente de haver offset de viewport ativo. O `UiLayer` faz TODA a tradução de/para seu próprio espaço de conteúdo offset-free internamente: subtrai `(x, y)` das coordenadas de mouse antes de repassar ao RmlUi, e soma `(x, y)` ao resultado bruto (espaço-conteúdo) de `get_element_box()` antes de devolver. **O host nunca precisa converter coordenadas pra input ou queries de geometria.** O ÚNICO lugar onde a convenção nativa bottom-up de viewport do OpenGL vaza é o parâmetro `target_h` de `set_viewport` (a altura da própria janela do host) -- necessário só porque o `glViewport` é inevitavelmente origem inferior-esquerda no passo final de composição (`gl_offset_y = target_h - y - h`); ver a seção "F3 implementation update" do ADR-0008 pra fórmula exata. `App` nunca tem offset (possui a janela inteira), então `App::get_element_box()` não precisa de tradução e `App` não ganha `set_viewport`.

**Por que F1/F2/F3 são documentadas juntas:** compartilham UM único sistema de coordenadas por design, e um erro de sinal em qualquer um dos três pontos de tradução (`MouseMove` de `process_event`, `get_element_box`, a fórmula de `set_viewport`) é exatamente a classe de bug silencioso de desalinhamento que o `set_click_callback` foi construído pra eliminar do lado do host -- reintroduzida dentro do próprio glintfx. O teste de integração `viewport_origin_sanity` exercita as três juntas como um cenário único com oráculo (clique-através-do-offset, geometria-através-do-offset e o posicionamento real de pixel no GL), não isoladamente.

## 11. Input hardening (v0.3.0) / Hardening de entrada (v0.3.0)

**EN:** Three public entry points now treat their arguments as coming from a less-trusted caller (RCSS/mod/dynamic source), fail-safe (reject and keep the previous state), never fail-open:

- **`load(rml_path)`** -- passing `nullptr` now returns `false` instead of constructing an internal `std::string(nullptr)` (undefined behaviour that used to crash the host via `std::terminate()`). Same guard already existed on the sibling entry points (`DataBinder`, `get_element_box`, `set_base_url`); it was only missing from `load()` itself.
- **`set_dp_ratio(ratio)`** (section 1) -- rejects `!std::isfinite(ratio) || ratio <= 0` and keeps the previous ratio instead of applying it. A `NaN` or `0` ratio previously propagated silently into every subsequent `dp`-unit layout computation.
- **`set_viewport(...)`** (section 0), both the 2-arg and the 5-arg (`UiLayer` letterbox) overloads -- rejects `w <= 0 || h <= 0` as a no-op, mirroring the guard already present on the `Resize` event path in `process_event()`.

Both `App` and `UiLayer` have parity on all three. Verified by `input_hardening_sanity` (embed, both `GLINTFX_BACKEND_GLFW` configs) and `app_input_hardening_smoke` (GLFW-only), both UBSan-clean under `GLINTFX_SANITIZE=ON`.

**EN:** Also new in v0.3.0: a `decorator: polygon(sides, fill[, rotation])` shape primitive is available for RCSS authored inside embedded documents, with the same `sides ∈ [3, 1024]` input validation as everything else in this section (invalid input is ignored with a warning, never a crash). `fill` accepts a solid `color` or, as of the following patch, a `radial-gradient(...)`/`linear-gradient(...)` (real per-pixel color ramp via RmlUi's own gradient shader, not a faceted approximation). It has no embed-specific behaviour -- see [`docs/effects.md`](effects.md) for the full syntax and how-to.

**EN:** Also fixed in that same patch: `load(rml_path)` no longer leaks the previously loaded document -- each call used to overwrite the internal document pointer without closing the old one first, leaking memory and leaving a "ghost" document still rendering underneath on every reload (e.g. a host navigating between menu screens on the same `UiLayer`). And `create_data_model`'s `bind_number/bind_string/bind_bool/bind_list`, if called twice with the same key before `load()`, used to free the first binding's backing cell before RmlUi's own bind call could fail cleanly -- a heap-use-after-free, not just a leak, reachable only via a caller bug (double-bind of the same key) but a real memory-safety issue. Both are now guarded (fail-safe: `load()` swaps documents only after the new one succeeds; `bind_*` rejects a duplicate key before touching anything). Neither is an embed-specific concern -- see `CHANGELOG.md`.

**PT:** Três pontos de entrada públicos agora tratam os argumentos como vindos de um chamador menos confiável (RCSS/mod/fonte dinâmica), fail-safe (rejeita e mantém o estado anterior), nunca fail-open:

- **`load(rml_path)`** -- passar `nullptr` agora retorna `false` em vez de construir um `std::string(nullptr)` interno (comportamento indefinido que antes derrubava o host via `std::terminate()`). O mesmo guard já existia nos pontos de entrada irmãos (`DataBinder`, `get_element_box`, `set_base_url`); faltava só no próprio `load()`.
- **`set_dp_ratio(ratio)`** (seção 1) -- rejeita `!std::isfinite(ratio) || ratio <= 0` e mantém o ratio anterior em vez de aplicá-lo. Um ratio `NaN` ou `0` antes se propagava silenciosamente para todo cálculo de layout subsequente com unidade `dp`.
- **`set_viewport(...)`** (seção 0), tanto a sobrecarga de 2 args quanto a de 5 args (letterbox do `UiLayer`) -- rejeita `w <= 0 || h <= 0` como no-op, replicando o guard já presente no caminho do evento `Resize` em `process_event()`.

`App` e `UiLayer` têm paridade nos três. Verificado por `input_hardening_sanity` (embed, nos 2 configs de `GLINTFX_BACKEND_GLFW`) e `app_input_hardening_smoke` (GLFW-only), ambos UBSan-limpos sob `GLINTFX_SANITIZE=ON`.

**PT:** Também novo na v0.3.0: uma shape primitive `decorator: polygon(lados, preenchimento[, rotação])` está disponível para RCSS autorado dentro de documentos embutidos, com a mesma validação de entrada `lados ∈ [3, 1024]` do resto desta seção (entrada inválida é ignorada com aviso, nunca crash). `preenchimento` aceita uma `cor` sólida ou, a partir do patch seguinte, um `radial-gradient(...)`/`linear-gradient(...)` (rampa de cor real por-pixel via o próprio shader de gradiente do RmlUi, não uma aproximação facetada). Não tem comportamento específico de embed -- ver [`docs/effects.md`](effects.md) para a sintaxe completa e o how-to.

**PT:** Também corrigido nesse mesmo patch: `load(rml_path)` não vaza mais o documento carregado anteriormente -- cada chamada sobrescrevia o ponteiro interno do documento sem fechar o antigo primeiro, vazando memória e deixando um documento "fantasma" ainda renderizando por baixo a cada reload (ex.: um host navegando entre telas de menu na mesma `UiLayer`). E os `bind_number/bind_string/bind_bool/bind_list` do `create_data_model`, se chamados duas vezes com a mesma chave antes do `load()`, liberavam a célula da primeira ligação antes do bind do próprio RmlUi conseguir falhar de forma limpa -- um heap-use-after-free, não só um vazamento, alcançável apenas via um erro do chamador (bind duplo da mesma chave), mas um problema real de segurança de memória. Ambos agora têm guard (fail-safe: `load()` só troca documentos após o novo carregar com sucesso; `bind_*` rejeita chave duplicada antes de tocar em qualquer coisa). Nenhum dos dois é uma preocupação específica de embed -- ver `CHANGELOG.md`.

## 12. Scrolling (v0.4.0) / Rolagem (v0.4.0)

**EN:** Two additions for `overflow-y: auto`/`overflow-x: auto` containers, both non-breaking:

- **Wheel forwarding** -- new `UiEvent::Type::MouseWheel` (`x`/`y` fields reused as scroll delta, not pointer position; `modifiers` reused as-is), forwarded by `UiLayer::process_event` to `Rml::Context::ProcessMouseWheel`. Scrolls the closest scrollable ancestor of the **hovered** element (not the focused one) -- send a `MouseMove` to the target position immediately before `MouseWheel` if your host does not already track hover via its own per-frame move events; a hover with no scrollable ancestor is a safe no-op. **`App` has no `process_event` at all (a pre-existing gap, not new in this release) -- wheel forwarding is `UiLayer`-only.** Full semantics, with exact RmlUi source references, are documented on `UiEvent::Type::MouseWheel` in `ui_event.hpp`.
- **Programmatic scroll** -- five `bool`-returning, `id`-lookup methods with parity across `UiLayer` and `App`: `scroll_element_into_view(id, align_with_top=true)`, `get_element_scroll_top`/`set_element_scroll_top(id, float&)`, and the rest of the scroll-metrics trio, `get_element_scroll_height`/`get_element_client_height` (for a custom scrollbar thumb/percentage: `scroll_fraction = scroll_top / (scroll_height - client_height)`). All five are null-id/unknown-id/no-document-safe; `set_element_scroll_top` rejects non-finite values. Unlike wheel forwarding, these work on `App` too (no `process_event` dependency).

Verified by `scroll_sanity` (embed, both `GLINTFX_BACKEND_GLFW` configs) and `app_scroll_smoke` (GLFW-only, programmatic-scroll parity for `App`). See `CHANGELOG.md` for the full write-up.

**PT:** Duas adições para containers `overflow-y: auto`/`overflow-x: auto`, ambas não-breaking:

- **Encaminhamento de wheel** -- novo `UiEvent::Type::MouseWheel` (campos `x`/`y` reusados como delta de rolagem, não posição do ponteiro; `modifiers` reusado como está), repassado por `UiLayer::process_event` para `Rml::Context::ProcessMouseWheel`. Rola o ancestral rolável mais próximo do elemento em **hover** (não do elemento com foco) -- envie um `MouseMove` para a posição-alvo imediatamente antes do `MouseWheel` se o seu host não já rastrear hover via seus próprios eventos de move por-frame; um hover sem ancestral rolável é um no-op seguro. **`App` não tem `process_event` nenhum (uma lacuna pré-existente, não nova nesta release) -- o encaminhamento de wheel é exclusivo do `UiLayer`.** A semântica completa, com referências exatas do source do RmlUi, está documentada em `UiEvent::Type::MouseWheel` em `ui_event.hpp`.
- **Scroll programático** -- cinco métodos retornando `bool`, busca-por-id, com paridade entre `UiLayer` e `App`: `scroll_element_into_view(id, align_with_top=true)`, `get_element_scroll_top`/`set_element_scroll_top(id, float&)`, e o resto do trio de métricas de rolagem, `get_element_scroll_height`/`get_element_client_height` (para um thumb/percentual de scrollbar customizado: `fracao_rolada = scroll_top / (scroll_height - client_height)`). Os cinco são seguros com id nulo/desconhecido/sem-documento; `set_element_scroll_top` rejeita valores não-finitos. Diferente do encaminhamento de wheel, esses funcionam também no `App` (sem dependência de `process_event`).

Verificado por `scroll_sanity` (embed, nos 2 configs de `GLINTFX_BACKEND_GLFW`) e `app_scroll_smoke` (GLFW-only, paridade de scroll programático pro `App`). Ver `CHANGELOG.md` para a descrição completa.

## 13. Extended navigation keys and click info (v0.5.0) / Teclas de navegação estendidas e click info (v0.5.0)

**EN:** Two additions, both purely additive to the API described in sections 5 and 10:

**(a) `enum class Key` gained 5 text-editing/document-navigation values:** `Delete`, `Home`, `End`, `PageUp`, `PageDown` (`glintfx/include/glintfx/ui_event.hpp`), mapped in `to_rml_key()` (`glintfx/src/ui_layer.cpp`) to RmlUi's own `KI_DELETE`/`KI_HOME`/`KI_END`/`KI_PRIOR`/`KI_NEXT`. They flow through `process_event`'s existing `UiEvent::Type::Key` path exactly like the pre-existing `Key::Up/Down/Left/Right/Enter/Escape/Tab` documented in section 5 -- no new event type, no new method. Whether they *move* anything still depends on the target document declaring `tabindex`/`nav` (same "not yet movement" caveat as section 5); glintfx only forwards the key. Letters/digits (e.g. text-field shortcuts) remain out of scope -- see `AUD-PUB-6` in the `TODO.md` INBOX.

**(b) `set_click_info_callback(std::function<void(const ClickInfo&)>)`** on both `UiLayer` and `App` (`glintfx/include/glintfx/click_info.hpp`) -- a **second, additive click channel**. It does **not** replace `set_click_callback` (section 10, id-only): both can be registered at the same time, and both keep firing independently on the same click.

```cpp
struct ClickInfo {
  const char* id;           // nearest ancestor-or-self id, "" if none (same lifetime
                             // contract as set_click_callback's element_id: valid only
                             // for the duration of the callback)
  int         button;       // 0=left, 1=right, 2=middle
  float       x, y;         // window-space physical pixels, same convention as
                             // get_element_box() (section 10)
  bool        double_click; // true only for Rml::EventId::Dblclick
};
```

- **`button` is functional for all three mouse buttons.** Left (`0`) comes from RmlUi's native `Click`/`Dblclick` events. Right (`1`) and middle (`2`) do **not** exist as native RmlUi click events -- glintfx synthesizes them from a paired `Mousedown`+`Mouseup`: on a non-primary `Mousedown` it remembers the resolved element id and button; if the paired `Mouseup` (same button) resolves to the **same** element, the callback fires once using that `Mouseup`'s own button/coordinates. A down-on-A/drag/up-on-B sequence does **not** fire -- this mirrors RmlUi's own left-click rule (`Context::ProcessMouseButtonUp` only dispatches `Click` when the press target is still the active/hover element at release).
- **`double_click` is left-only.** RmlUi has no `Dblclick` equivalent for non-primary buttons, and inventing detection via a custom timer was deliberately rejected as out of scope -- a right/middle `ClickInfo` always reports `double_click=false`, even for two fast right-clicks on the same element.
- **Coordinates follow the section-10 contract**: window/render-target physical pixels, top-left origin, y-down. `UiLayer::set_click_info_callback` adds the active sub-viewport offset internally (same translation as `get_element_box`); `App` needs no translation (owns the whole window).

```cpp
// C++ -- register both channels; they coexist
ui.set_click_callback([](const char* id) {
  // cheap, id-only routing
});
ui.set_click_info_callback([](const glintfx::ClickInfo& info) {
  if (info.button == 1) {
    open_context_menu(info.id, info.x, info.y);   // right-click
  } else if (info.double_click) {
    open_item(info.id);                            // left double-click
  }
});
```

Verified by `click_callback_sanity` (single click; the exact 4-event double-click ordering; right/middle-click parity with `button`/`double_click`; drag-off non-firing; no regression of the id-only channel).

**PT:** Duas adições, ambas puramente aditivas à API descrita nas seções 5 e 10:

**(a) `enum class Key` ganhou 5 valores de edição de texto/navegação de documento:** `Delete`, `Home`, `End`, `PageUp`, `PageDown` (`glintfx/include/glintfx/ui_event.hpp`), mapeados em `to_rml_key()` (`glintfx/src/ui_layer.cpp`) para os próprios `KI_DELETE`/`KI_HOME`/`KI_END`/`KI_PRIOR`/`KI_NEXT` do RmlUi. Fluem pelo caminho já existente `UiEvent::Type::Key` de `process_event`, exatamente como os `Key::Up/Down/Left/Right/Enter/Escape/Tab` pré-existentes documentados na seção 5 -- nenhum tipo de evento novo, nenhum método novo. Se algo de fato *se move* continua dependendo do documento-alvo declarar `tabindex`/`nav` (mesma ressalva "ainda não o movimento" da seção 5); o glintfx só encaminha a tecla. Letras/dígitos (ex.: atalhos de campo de texto) seguem fora de escopo -- ver `AUD-PUB-6` na INBOX do `TODO.md`.

**(b) `set_click_info_callback(std::function<void(const ClickInfo&)>)`** em `UiLayer` e `App` (`glintfx/include/glintfx/click_info.hpp`) -- um **segundo canal de clique, aditivo**. **Não** substitui o `set_click_callback` (seção 10, só-id): os dois podem ser registrados ao mesmo tempo, e ambos continuam disparando independentemente no mesmo clique.

```cpp
struct ClickInfo {
  const char* id;           // id do ancestral-ou-o-próprio mais próximo, "" se nenhum
                             // (mesmo contrato de lifetime do element_id de
                             // set_click_callback: válido só durante a chamada)
  int         button;       // 0=esquerdo, 1=direito, 2=meio
  float       x, y;         // pixels físicos espaço-janela, mesma convenção do
                             // get_element_box() (seção 10)
  bool        double_click; // true só para Rml::EventId::Dblclick
};
```

- **`button` é funcional para os três botões do mouse.** O esquerdo (`0`) vem dos eventos nativos `Click`/`Dblclick` do RmlUi. Direito (`1`) e meio (`2`) **não existem** como eventos nativos de clique do RmlUi -- o glintfx os sintetiza a partir de um par `Mousedown`+`Mouseup`: num `Mousedown` não-primário ele lembra o id do elemento resolvido e o botão; se o `Mouseup` pareado (mesmo botão) resolve para o **mesmo** elemento, o callback dispara uma vez usando o botão/coordenadas do próprio `Mouseup`. Uma sequência down-em-A/arrasta/up-em-B **não** dispara -- espelha a própria regra de clique esquerdo do RmlUi (`Context::ProcessMouseButtonUp` só despacha `Click` quando o alvo da pressão ainda é o elemento ativo/hover na soltura).
- **`double_click` é esquerdo-apenas.** O RmlUi não tem equivalente de `Dblclick` para botões não-primários, e inventar detecção via timer customizado foi deliberadamente rejeitada como fora de escopo -- um `ClickInfo` de direito/meio sempre reporta `double_click=false`, mesmo para dois cliques direitos rápidos no mesmo elemento.
- **As coordenadas seguem o contrato da seção 10**: pixels físicos do render-target/janela, origem superior-esquerda, y pra baixo. `UiLayer::set_click_info_callback` soma o offset da sub-viewport ativa internamente (mesma tradução do `get_element_box`); o `App` não precisa de tradução (dono da janela inteira).

```cpp
// C++ -- registra os dois canais; eles coexistem
ui.set_click_callback([](const char* id) {
  // roteamento barato, só-id
});
ui.set_click_info_callback([](const glintfx::ClickInfo& info) {
  if (info.button == 1) {
    abrir_menu_contexto(info.id, info.x, info.y);   // clique direito
  } else if (info.double_click) {
    abrir_item(info.id);                             // duplo-clique esquerdo
  }
});
```

Verificado por `click_callback_sanity` (clique simples; a ordem exata dos 4 eventos de duplo-clique; paridade de clique direito/meio com `button`/`double_click`; não-disparo em drag-off; não-regressão do canal só-id).

## 14. Scroll callback (v0.6.0) / Callback de rolagem (v0.6.0)

**EN:** `set_scroll_callback(std::function<void(const char* id)>)` on both `UiLayer` and `App` -- purely additive follow-up to section 12. Wraps RmlUi's native `Rml::EventId::Scroll`, fired whenever an element's OWN scroll offset changes.

- **Fires from three sources**, none distinguished from one another: mouse-wheel scrolling (section 12), dragging/clicking the native scrollbar (thumb, track, or arrows -- section "How-to: style scrollbars" in `docs/effects.md` for how those elements look), and the programmatic `scroll_element_into_view()`/`set_element_scroll_top()` (also section 12).
- **id-only, no offset payload.** Reports the id of the nearest ancestor-or-self of the scrolled element (`""` if none) -- same lifetime contract as `set_click_callback`'s `element_id` (valid only for the duration of the call). The underlying `Rml::EventId::Scroll` carries no numeric data, so if the offset is needed, call `get_element_scroll_top(id)` (section 12) from inside the callback.
- **Deduplicated by RmlUi itself** against the element's current offset: setting the same scroll position twice in a row does not re-fire. Null/empty callback is a safe no-op; a reentrancy guard (the registered functor is copied before invocation) protects against the callback replacing itself mid-call.
- **glintfx does not play audio.** This callback is the intended hook for a host to trigger its own scroll sound effect -- see the example below.

```cpp
// C++ -- play a scroll sound, debounced per-gesture on the host side
ui.set_scroll_callback([](const char* id) {
  static std::string last_id;
  if (id != last_id) {          // coarse debounce: only on a NEW element scrolling
    last_id = id;
    audio_play("scroll.wav");
  }
});
```

**Two gotchas:**

**(a) Multiple fires per wheel gesture.** Wheel scrolling in RmlUi is a smoothscroll animation: the callback fires **once per animated step**, not once when the scroll settles. A host that wants exactly one sound per wheel gesture must debounce/coalesce on its own side (as in the example above), not assume one call per user action.

**(b) Recursion risk with non-convergent writes.** `scroll_element_into_view()` and `set_element_scroll_top()` (section 12) are **synchronous** -- they dispatch the underlying `Scroll` event on the SAME call stack before returning, which re-enters this callback. Calling either of them again from inside the callback with a value that does **not converge** to the element's current offset (e.g. a dynamic clamp/snap that always computes something different from what it just read) recurses without bound -> stack overflow. It is safe to call them from inside the callback as long as the written value converges -- RmlUi's own dedup (same offset twice does not re-fire) breaks the cycle, typically within 1-2 iterations for a constant target.

`set_click_callback`/`set_click_info_callback` (section 10 / section 13) are untouched and keep firing independently -- `set_scroll_callback` is a separate, additive channel.

Verified by `scroll_sanity` (case E: fires with the scrolled element's id from wheel scrolling and from the programmatic `set_element_scroll_top`, dedup on a same-value set, and a safe no-op after clearing the callback; case E5: a dedicated case for the convergent-recursion pattern in gotcha (b), asserting it settles in a bounded number of iterations rather than overflowing -- native scrollbar thumb/track/arrow interaction dispatches the same `Rml::EventId::Scroll` internally but is not separately simulated in this test); see `CHANGELOG.md` v0.6.0 for the full write-up. Scrollbar RCSS *appearance* (thickness, thumb/arrow color, sizing) is a styling concern, not this callback -- see `docs/effects.md`'s "How-to: style scrollbars".

**PT:** `set_scroll_callback(std::function<void(const char* id)>)` em `UiLayer` e `App` -- desdobramento puramente aditivo da seção 12. Encapsula o evento nativo `Rml::EventId::Scroll` do RmlUi, disparado sempre que o offset de rolagem PRÓPRIO de um elemento muda.

- **Dispara de três fontes**, nenhuma distinguida da outra: rolagem por wheel do mouse (seção 12), arrastar/clicar a scrollbar nativa (thumb, trilho ou setas -- ver a seção "How-to: estilizar barras de rolagem (scrollbars)" em `docs/effects.md` para a aparência desses elementos), e as chamadas programáticas `scroll_element_into_view()`/`set_element_scroll_top()` (também seção 12).
- **Só-id, sem payload de offset.** Reporta o id do ancestral-ou-o-próprio mais próximo do elemento rolado (`""` se nenhum) -- mesmo contrato de lifetime do `element_id` de `set_click_callback` (válido só durante a chamada). O `Rml::EventId::Scroll` subjacente não carrega dado numérico nenhum, então, se o offset for necessário, chame `get_element_scroll_top(id)` (seção 12) de dentro do callback.
- **Deduplicado pelo próprio RmlUi** contra o offset corrente do elemento: definir a mesma posição de rolagem duas vezes seguidas não redispara. Callback nulo/vazio é no-op seguro; uma guarda de reentrância (o functor registrado é copiado antes de invocar) protege contra o callback se substituir no meio da própria chamada.
- **O glintfx não toca áudio.** Este callback é o gancho pretendido para um host disparar seu próprio efeito sonoro de rolagem -- ver o exemplo abaixo.

```cpp
// C++ -- toca um som de rolagem, debounced por-gesto do lado do host
ui.set_scroll_callback([](const char* id) {
  static std::string last_id;
  if (id != last_id) {          // debounce grosseiro: só quando um elemento NOVO rola
    last_id = id;
    audio_play("scroll.wav");
  }
});
```

**Duas pegadinhas:**

**(a) Múltiplos disparos por gesto de wheel.** A rolagem por wheel no RmlUi é uma animação de smoothscroll: o callback dispara **uma vez por passo animado**, não uma vez quando a rolagem assenta. Um host que quer exatamente um som por gesto de wheel precisa debounce/coalescer do próprio lado (como no exemplo acima), não assumir uma chamada por ação do usuário.

**(b) Risco de recursão com escritas não-convergentes.** `scroll_element_into_view()` e `set_element_scroll_top()` (seção 12) são **síncronos** -- despacham o evento `Scroll` subjacente na MESMA pilha de chamada antes de retornar, o que reentra neste callback. Chamar qualquer um deles novamente de dentro do callback com um valor que NÃO converge para o offset atual do elemento (ex.: um clamp/snap dinâmico que sempre calcula algo diferente do que acabou de ler) recursa sem limite -> stack overflow. É seguro chamá-los de dentro do callback desde que o valor escrito convirja -- o próprio dedup do RmlUi (mesmo offset duas vezes não redispara) quebra o ciclo, tipicamente em 1-2 iterações para um alvo constante.

`set_click_callback`/`set_click_info_callback` (seção 10 / seção 13) seguem intactos e continuam disparando independentemente -- `set_scroll_callback` é um canal separado, aditivo.

Verificado por `scroll_sanity` (caso E: dispara com o id do elemento rolado a partir de wheel e do `set_element_scroll_top` programático, dedup num set de mesmo valor, e no-op seguro após limpar o callback; caso E5: um caso dedicado para o padrão de recursão convergente da pegadinha (b), afirmando que ela assenta num número limitado de iterações em vez de estourar a pilha -- a interação com thumb/trilho/setas da scrollbar nativa despacha o mesmo `Rml::EventId::Scroll` internamente mas não é simulada separadamente neste teste); ver `CHANGELOG.md` v0.6.0 para a descrição completa. A *aparência* RCSS da scrollbar (espessura, cor de thumb/setas, dimensionamento) é uma questão de estilização, não deste callback -- ver "How-to: estilizar barras de rolagem (scrollbars)" em `docs/effects.md`.

## See also / Veja também

- [ADR-0008](adr/0008-embed-guest-mode.md): embed/guest mode decision, including the GL state save and restore clause (d). / decisão do embed/guest mode, incluindo a cláusula (d) de save e restore de estado GL.
- [`README.md`](../README.md): library overview and the standalone `glintfx::App` path. / visão geral da lib e o caminho standalone `glintfx::App`.
- [`docs/effects.md`](effects.md): RCSS effect syntax used inside embedded documents. / sintaxe de efeitos RCSS usada dentro de documentos embutidos.
