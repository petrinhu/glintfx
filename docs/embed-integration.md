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
- `UiLayerConfig{logical_width = 1280, logical_height = 720}` (`glintfx/include/glintfx/ui_layer.hpp`) is only the **initial** context size passed at attach. `set_viewport` overrides it.
- Composition always targets the **window backbuffer (FBO 0)** with viewport origin hardcoded at `(0,0)`. A custom host FBO bound before `render()` is **not** used as the composition target. Sub region compositing is not supported.
- **dp_ratio (density-independent pixel ratio).** `UiLayerConfig::dp_ratio` (default `1.0f`) and `UiLayer::set_dp_ratio(float)` expose `Rml::Context::SetDensityIndependentPixelRatio`. When set, the RCSS `dp` unit scales: `1dp = dp_ratio physical px`. The `px` RCSS unit is always one physical pixel and is NOT affected by dp_ratio.
  - **Recommended pattern:** author RCSS in `dp` units at a fixed logical canvas (e.g. 960×540), call `set_viewport(1920, 1080)` (physical backbuffer), and `set_dp_ratio(1920.0f / 960.0f)` = `2.0f`. RmlUi scales the layout to fill the physical backbuffer without any manual RCSS edits.
  - `App` has parity: `AppConfig::dp_ratio` and `App::set_dp_ratio(float)`.
  - Verified: `dp_ratio_sanity` test renders a 100dp×100dp white box; scale factor at ratio=2.0 vs ratio=1.0 is exactly **4.0×** (200×200 px vs 100×100 px).

**PT:**
- `set_viewport(w, h)` espera os **pixels reais do backbuffer**. Os mesmos `(w, h)` alimentam duas coisas acopladas, 1:1: a dimensão do contexto RmlUi via `Engine::set_viewport` -> `context->SetDimensions` e o viewport GL via `render_compose` -> `begin_frame_compose`.
- `UiLayerConfig{logical_width = 1280, logical_height = 720}` é só a dimensão **inicial** do contexto. `set_viewport` a sobrescreve.
- A composição mira sempre o **backbuffer da janela (FBO 0)** com origem do viewport fixa em `(0,0)`. Composição em sub região não é suportada.
- **dp_ratio (density-independent pixel ratio).** `UiLayerConfig::dp_ratio` (padrão `1.0f`) e `UiLayer::set_dp_ratio(float)` expõem `Rml::Context::SetDensityIndependentPixelRatio`. Quando definido, a unidade RCSS `dp` escala: `1dp = dp_ratio px físicos`. A unidade `px` do RCSS é sempre um pixel físico e NÃO é afetada pelo dp_ratio.
  - **Padrão recomendado:** autore o RCSS em unidades `dp` num canvas lógico fixo (ex.: 960×540), chame `set_viewport(1920, 1080)` (backbuffer físico) e `set_dp_ratio(1920.0f / 960.0f)` = `2.0f`. O RmlUi escala o layout para preencher o backbuffer físico sem nenhuma edição de RCSS.
  - `App` tem paridade: `AppConfig::dp_ratio` e `App::set_dp_ratio(float)`.
  - Verificado: teste `dp_ratio_sanity` renderiza box branco 100dp×100dp; fator de escala em ratio=2.0 vs ratio=1.0 é exatamente **4.0×** (200×200 px vs 100×100 px).

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
- `load(rml_path)` forwards to `LoadDocument(rml_path)` (`glintfx/src/bootstrap.cpp`). RmlUi resolves relative URLs (the `<link href>` in the RML, and `@font-face src` / `image()` / decorator paths in the RCSS) **relative to the document URL** (which is the path passed to `load()`), **not** relative to the process working directory.
- **`set_asset_base_url(const char*)`.** glintfx installs a custom `Rml::FileInterface` (`glintfx/src/base_url_file_interface.hpp`) before `Rml::Initialise()`. When `set_asset_base_url(url)` is called, all subsequent relative paths passed to `Open()` are resolved as `url/path` instead of being looked up from the process CWD. Absolute paths (starting with `/`) bypass the prefix. A fallback to the raw path is attempted if the prefixed open fails.
  - **Call before `load()`** so the document itself also resolves via the base URL.
  - Pass `nullptr` or `""` to clear.
  - `App::set_asset_base_url(const char*)` provides parity with `UiLayer`.
  - Verified: `base_url_sanity` test loads `embed_scene.rml` (not present in the CWD) successfully after `set_asset_base_url("base_url_assets")`. The rendered frame passes all three pixel statistics.
- **How to point at fonts and sprites (traditional pattern):** call `load("<dir>/cockpit.rml")` and place `cockpit.rcss`, the font file, and sprites at paths the RML/RCSS reference relative to their own location. For example `@font-face { src: "fonts/PixelOperatorMono.ttf"; }` resolves to `<dir>/fonts/...`. This pattern works without any base URL.
- **Cautions:** absolute paths in RmlUi URLs may lose the leading slash during canonicalisation, so prefer relative paths. The base URL override is most useful for absolute asset roots or when the process CWD cannot be controlled.

**PT:**
- `load(rml_path)` encaminha para `LoadDocument(rml_path)` (`glintfx/src/bootstrap.cpp`). O RmlUi resolve URLs relativas **relativo à URL do documento** (o path passado a `load()`), **não** relativo ao diretório de trabalho do processo.
- **`set_asset_base_url(const char*)`.** O glintfx instala um `Rml::FileInterface` customizado (`glintfx/src/base_url_file_interface.hpp`) antes de `Rml::Initialise()`. Quando `set_asset_base_url(url)` é chamado, todos os caminhos relativos subsequentes passados a `Open()` são resolvidos como `url/path` em vez de serem buscados no CWD do processo. Caminhos absolutos (começando com `/`) ignoram o prefixo. Um fallback para o path bruto é tentado se o open prefixado falhar.
  - **Chame antes de `load()`** para que o documento em si também resolva via o base URL.
  - Passe `nullptr` ou `""` para limpar.
  - `App::set_asset_base_url(const char*)` tem paridade com `UiLayer`.
  - Verificado: teste `base_url_sanity` carrega `embed_scene.rml` (ausente no CWD) com sucesso após `set_asset_base_url("base_url_assets")`. O frame renderizado passa as três estatísticas de pixel.
- **Como apontar fontes e sprites (padrão tradicional):** chame `load("<dir>/cockpit.rml")` e coloque `cockpit.rcss`, o arquivo de fonte e os sprites em caminhos que a RML/RCSS referencia relativo à própria localização. Por exemplo `@font-face { src: "fonts/PixelOperatorMono.ttf"; }` resolve para `<dir>/fonts/...`. Este padrão funciona sem nenhum base URL.
- **Cautelas:** caminhos absolutos em URLs do RmlUi podem perder a barra inicial na canonicalização; prefira caminhos relativos. O override de base URL é mais útil para raízes de asset absolutas ou quando o CWD do processo não pode ser controlado.

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

### Cockpit example / Exemplo do cockpit

**EN:** A game HUD with a numeric HP bar, verb and target strings, and a scrolling action log. The `data-for` attribute iterates the list; `{{line}}` is the per-item alias.

**PT:** Um HUD de jogo com barra de HP numérica, strings de verbo e alvo e um log de ações rolante. O atributo `data-for` itera a lista; `{{line}}` é o alias por item.

```cpp
// C++ -- create and bind before load()
ui.create_data_model("hud");
ui.bind_number("hp",     100.0);
ui.bind_string("verb",   "");
ui.bind_string("target", "");
ui.bind_list  ("log");

ui.load("cockpit.rml");   // views compile here

// per-frame update
ui.set_number("hp",     player.hp);
ui.set_string("verb",   action.verb);
ui.set_string("target", action.target);
const char* lines[] = { log[0].c_str(), log[1].c_str(), log[2].c_str() };
ui.set_list("log", lines, 3);
```

```html
<!-- cockpit.rml excerpt -->
<body data-model="hud">
  <p>HP {{hp}}</p>
  <p>{{verb}} -- {{target}}</p>
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
2. **Custom host FBO target plus configurable viewport origin.** Today `render()` always composes onto FBO 0 at origin `(0,0)`. Compositing into a host owned offscreen FBO, or into a sub region, is not supported.
3. ~~**Asset base URL override.**~~ **Resolved in v0.2.2.** `UiLayer::set_asset_base_url(const char*)` and `App::set_asset_base_url(const char*)` are now available. See section 4 for the full contract.

**PT:** Features candidatas para uma versão futura:
1. ~~**Escala lógico para físico (dp_ratio).**~~ **Resolvido na v0.2.2.** `UiLayerConfig::dp_ratio`, `UiLayer::set_dp_ratio(float)`, `AppConfig::dp_ratio` e `App::set_dp_ratio(float)` estão disponíveis. Ver seção 1 para o contrato completo.
2. **Alvo de FBO custom do host mais origem de viewport configurável.** Hoje o `render()` sempre compõe no FBO 0 na origem `(0,0)`. Compor num FBO offscreen do host, ou numa sub região, não é suportado.
3. ~~**Override de base URL de assets.**~~ **Resolvido na v0.2.2.** `UiLayer::set_asset_base_url(const char*)` e `App::set_asset_base_url(const char*)` estão disponíveis. Ver seção 4 para o contrato completo.

## See also / Veja também

- [ADR-0008](adr/0008-embed-guest-mode.md): embed/guest mode decision, including the GL state save and restore clause (d). / decisão do embed/guest mode, incluindo a cláusula (d) de save e restore de estado GL.
- [`README.md`](../README.md): library overview and the standalone `glintfx::App` path. / visão geral da lib e o caminho standalone `glintfx::App`.
- [`docs/effects.md`](effects.md): RCSS effect syntax used inside embedded documents. / sintaxe de efeitos RCSS usada dentro de documentos embutidos.
