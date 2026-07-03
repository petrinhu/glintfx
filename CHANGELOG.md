# Changelog

> **EN:** All notable changes to glintfx. Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); this project follows [Semantic Versioning](https://semver.org/).
> **PT:** Todas as mudanças notáveis do glintfx. Formato baseado em [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); o projeto segue [Versionamento Semântico](https://semver.org/).

---

## [Unreleased]

---

## [0.2.5] - 2026-07-02

### Added / Adicionado

- **EN:** `set_click_callback(std::function<void(const char* element_id)>)` on both `UiLayer` and `App` -- fires on "click" (bubble phase, listener attached to the document root by `load()`), reporting the id of the clicked element, or the nearest ancestor id if the target itself has none, or `""` if no element in the ancestor chain has an id. No ordering constraint versus `load()` -- can be set before or after. Verified by `click_callback_sanity` and `app_click_callback_smoke`. Resolves `L1.11-CLICKCB`.
  **PT:** `set_click_callback(std::function<void(const char* element_id)>)` em `UiLayer` e `App` -- dispara em "click" (fase bubble, listener anexado à raiz do documento por `load()`), reportando o id do elemento clicado, ou o id do ancestral mais próximo se o próprio alvo não tiver, ou `""` se nenhum elemento na cadeia de ancestrais tiver id. Sem restrição de ordem vs. `load()` -- pode ser setado antes ou depois. Verificado por `click_callback_sanity` e `app_click_callback_smoke`. Resolve `L1.11-CLICKCB`.
- **EN:** `get_element_box(const char* id) -> ElementBox { bool found; float x, y, w, h; }` on both `UiLayer` and `App` -- returns the border-box geometry (`Rml::BoxArea::Border`) of an element by id, in window-space physical pixels, or `found=false` if the id does not exist or no document is loaded yet. New public header `glintfx/element_box.hpp`. Verified by `element_box_sanity` and `app_element_box_smoke`. Resolves `L1.12-ELEMBOX`.
  **PT:** `get_element_box(const char* id) -> ElementBox { bool found; float x, y, w, h; }` em `UiLayer` e `App` -- retorna a geometria border-box (`Rml::BoxArea::Border`) de um elemento por id, em pixels físicos espaço-janela, ou `found=false` se o id não existir ou nenhum documento estiver carregado ainda. Novo header público `glintfx/element_box.hpp`. Verificado por `element_box_sanity` e `app_element_box_smoke`. Resolve `L1.12-ELEMBOX`.
- **EN:** `UiLayer::set_viewport(int x, int y, int w, int h, int target_h)` -- places the composited UI within a sub-region of the host's window (letterbox), resolving the hardcoded-`(0,0)`-origin gap tracked as `GAP-2` since ADR-0008. `(x, y)` are top-down window-space (the same convention as `UiEvent`'s mouse coordinates and `get_element_box()`'s return value); `target_h` is the host's full render-target height, used only to convert internally to OpenGL's bottom-up viewport offset. The legacy `set_viewport(w, h)` (2-arg) is unchanged and equivalent to `(0, 0, w, h, h)`. `UiLayer`-only by design -- `App` always owns its whole window, so a sub-viewport offset is meaningless there (see ADR-0008 "F3 implementation update"). Verified by `viewport_origin_sanity`. Resolves `L1.13-VPORIGIN` and the origin half of `GAP-2` (the custom-host-FBO half of `GAP-2` remains open, see `docs/embed-integration.md` section 8).
  **PT:** `UiLayer::set_viewport(int x, int y, int w, int h, int target_h)` -- posiciona a UI composta numa sub-região da janela do host (letterbox), resolvendo a lacuna de origem hardcoded em `(0,0)` rastreada como `GAP-2` desde o ADR-0008. `(x, y)` são espaço-janela top-down (a mesma convenção das coordenadas de mouse do `UiEvent` e do retorno de `get_element_box()`); `target_h` é a altura total do render-target do host, usada só pra converter internamente para o offset de viewport bottom-up do OpenGL. O `set_viewport(w, h)` legado (2 args) é inalterado e equivalente a `(0, 0, w, h, h)`. Só em `UiLayer` por design -- o `App` sempre possui a janela inteira, então um offset de sub-viewport não faz sentido lá (ver "F3 implementation update" no ADR-0008). Verificado por `viewport_origin_sanity`. Resolve `L1.13-VPORIGIN` e a metade "origem" do `GAP-2` (a metade "FBO custom do host" do `GAP-2` segue em aberto, ver `docs/embed-integration.md` seção 8).
- **EN:** Nightly memory gate (`TST-L1-MEM`): CMake option `GLINTFX_SANITIZE` (ASan+UBSan+LSan) with suppression files for third-party noise, mirrored on GitHub Actions and Codeberg Forgejo Actions (cron + manual dispatch). Landed in this release window ahead of the API additions above -- not itself part of F1/F2/F3, noted here for release-window completeness. See `TESTES.md`.
  **PT:** Gate noturno de memória (`TST-L1-MEM`): opção CMake `GLINTFX_SANITIZE` (ASan+UBSan+LSan) com arquivos de supressão para ruído de terceiros, espelhado em GitHub Actions e Codeberg Forgejo Actions (cron + dispatch manual). Entrou nesta janela de release antes das adições de API acima -- não faz parte em si de F1/F2/F3, anotado aqui por completude da janela. Ver `TESTES.md`.

### Docs / Documentação

- **EN:** ADR-0008 -- new dated section "F3 implementation update -- configurable viewport origin (v0.2.5)", added after "F1 implementation constraints" without reopening the `Accepted` decision. `docs/embed-integration.md` -- new section 10 ("Click callback, element geometry, and the window-space coordinate contract"), consolidating the three v0.2.5 additions and their shared coordinate contract; sections 0, 1, 2, and 8 updated in place to point at it.
  **PT:** ADR-0008 -- nova seção datada "F3 implementation update -- configurable viewport origin (v0.2.5)", acrescentada após "F1 implementation constraints" sem reabrir a decisão `Accepted`. `docs/embed-integration.md` -- nova seção 10 ("Callback de clique, geometria de elemento e o contrato de coordenadas espaço-janela"), consolidando as três adições da v0.2.5 e o contrato de coordenadas compartilhado; seções 0, 1, 2 e 8 atualizadas no lugar para apontar para ela.

### Testing / Testes

- **EN:** 5 new tests this release (`click_callback_sanity`, `app_click_callback_smoke`, `element_box_sanity`, `app_element_box_smoke`, `viewport_origin_sanity`; 3 of the 5 run in both build configs). Suite size: **23 tests** with `GLINTFX_BACKEND_GLFW=ON`, **11** with `=OFF` (embed-only).
  **PT:** 5 testes novos nesta release (`click_callback_sanity`, `app_click_callback_smoke`, `element_box_sanity`, `app_element_box_smoke`, `viewport_origin_sanity`; 3 dos 5 rodam nos 2 configs de build). Tamanho da suíte: **23 testes** com `GLINTFX_BACKEND_GLFW=ON`, **11** com `=OFF` (embed-only).

[0.2.5]: https://codeberg.org/petrinhu/glintfx/releases/tag/v0.2.5

---

## [0.2.4] - 2026-07-01

### Added / Adicionado

- **EN:** Embedded User-Agent stylesheet: `display: block` defaults for structural elements (`div, p, h1..h6, ul, ol, li, section, article, header, footer, nav, main`), merged into every document loaded via `load()` on both `App` and `UiLayer` (shared code path in `Bootstrap::load()`). Applied as a low-specificity base -- any author rule of equal or higher specificity overrides it. Verified by `ua_stylesheet_sanity`. Documented in `docs/embed-integration.md` section 9.
  **PT:** Stylesheet User-Agent embutida: defaults de `display: block` para elementos estruturais (`div, p, h1..h6, ul, ol, li, section, article, header, footer, nav, main`), mesclada em todo documento carregado via `load()` tanto em `App` quanto em `UiLayer` (caminho de código compartilhado em `Bootstrap::load()`). Aplicada como base de baixa especificidade -- qualquer regra do autor com especificidade igual ou maior a sobrepõe. Verificado por `ua_stylesheet_sanity`. Documentado em `docs/embed-integration.md` seção 9.

### Fixed / Corrigido

- **EN:** `version()` returned a hardcoded `"0.1.0"`, stale since v0.1.1; now reads `GLINTFX_VERSION` generated by CMake from `project(glintfx VERSION ...)` -- single source of truth.
  **PT:** `version()` retornava `"0.1.0"` hardcoded, defasado desde a v0.1.1; agora lê `GLINTFX_VERSION` gerado pelo CMake a partir de `project(glintfx VERSION ...)` -- fonte única.
- **EN:** Test-count comment in the CI workflows (`.forgejo/`, `.github/`) was stale ("10 tests" / "5 embed tests", from when the suite had 10/5); corrected to the current values (18/8). Comment-only, no change in CI behaviour.
  **PT:** Comentário de contagem de testes nos workflows de CI (`.forgejo/`, `.github/`) estava defasado ("10 testes" / "5 testes embed", de quando a suíte tinha 10/5); corrigido para os valores atuais (18/8). Comment-only, sem mudança de comportamento de CI.

---

## [0.2.3] - 2026-06-30

### Added / Adicionado

- **EN:** Data-model binding via `create_data_model(name)` + `bind_number / bind_string / bind_bool / bind_list(key)` + `set_*(key, value)` on both `UiLayer` and `App`. Mandatory call order: `create_data_model` → `bind_*` → `load()` → `set_*` (calling `bind_*` after `load()` returns `false` -- RmlUi compiles views at load time). `bind_list` supports string-list variables iterable in RML via `data-for="item : list"` and `{{item}}` interpolation -- enables scrolling log panels, menus, and inventory lists. Memory is owned by the engine (`DataBinder` uses one `std::unique_ptr` per key in `std::map` for stable addresses); the consumer passes values by copy only. Verified by `data_model_smoke`, `data_model_scalar`, and `data_model_list` tests.
  **PT:** Ligacao de data-model via `create_data_model(name)` + `bind_number / bind_string / bind_bool / bind_list(key)` + `set_*(key, value)` em `UiLayer` e `App`. Ordem obrigatoria: `create_data_model` → `bind_*` → `load()` → `set_*` (chamar `bind_*` apos `load()` retorna `false` -- o RmlUi compila views no momento do load). `bind_list` suporta variaveis de lista de strings iteraveis no RML via `data-for="item : list"` e interpolacao `{{item}}` -- permite paineis de log rolante, menus e listas de inventario. A memoria e de posse do engine (`DataBinder` usa um `std::unique_ptr` por chave em `std::map` para enderecos estaveis); o consumidor passa valores apenas por copia. Verificado pelos testes `data_model_smoke`, `data_model_scalar` e `data_model_list`.

### Changed / Alterado

- **EN:** `App::load()` and `UiLayer::load()` now return `bool` (true on success, false on failure) -- previously `void`. Source-compatible: callers that discard the return value continue to compile without change. This is a signature and ABI change; callers that store the return type or use the methods in a `decltype` context must be updated.
  **PT:** `App::load()` e `UiLayer::load()` agora retornam `bool` (true em sucesso, false em falha) -- anteriormente `void`. Compativel com source: callers que descartam o retorno continuam compilando sem alteracao. Esta e uma mudanca de assinatura e ABI; callers que armazenam o tipo de retorno ou usam os metodos em contexto `decltype` precisam ser atualizados.

### Fixed / Corrigido

- **EN:** Texture loading now supports **PNG and JPG** (previously only TGA decoded reliably). Decoding is handled by **stb_image** (`glintfx/third_party/stb/stb_image.h`) via an override of `Rml::RenderInterface::LoadTexture`. After decoding, the alpha channel is **premultiplied in-place** (`out.rgb = in.rgb * in.a / 255`) before GPU upload -- required for correct compositing with the GL3 premultiplied-alpha blend (`GL_ONE, GL_ONE_MINUS_SRC_ALPHA`) used by the renderer. Without premultiplication a PNG with straight alpha produces a bright halo around transparent regions. On decode failure the backend falls back to the upstream RmlUi TGA loader so existing TGA assets do not regress. Verified by `texture_png_alpha` test. Documented in `docs/embed-integration.md` section 7.
  **PT:** O carregamento de texturas agora suporta **PNG e JPG** (anteriormente apenas o TGA decodificava de forma confiavel). A decodificacao e feita pelo **stb_image** (`glintfx/third_party/stb/stb_image.h`) via override de `Rml::RenderInterface::LoadTexture`. Apos decodificar, o canal alpha e **premultiplicado in-place** (`out.rgb = in.rgb * in.a / 255`) antes do upload para a GPU -- necessario para composicao correta com o blend premultiplied-alpha GL3 (`GL_ONE, GL_ONE_MINUS_SRC_ALPHA`) usado pelo renderer. Sem premultiplicacao, um PNG com alpha straight produz um halo claro ao redor das regioes transparentes. Em caso de falha de decode, o backend cai no loader TGA do upstream RmlUi para que assets TGA existentes nao regridam. Verificado pelo teste `texture_png_alpha`. Documentado em `docs/embed-integration.md` secao 7.

---

## [0.2.2] - 2026-06-30

### Added / Adicionado

- **EN:** `UiLayerConfig::dp_ratio` (field, default `1.0f`) and `UiLayer::set_dp_ratio(float)` (runtime setter): expose `Rml::Context::SetDensityIndependentPixelRatio` to host consumers. When set, the RCSS `dp` unit scales to `dp_ratio` physical pixels (the `px` unit is unaffected). Recommended pattern: author RCSS at a fixed logical size using `dp` units, call `set_viewport(real_w, real_h)` and `set_dp_ratio(real_w / logical_w)`. `App` parity: `AppConfig::dp_ratio` and `App::set_dp_ratio(float)`. Verified by `dp_ratio_sanity` (100dp×100dp white box; scale factor 4.00x at ratio=2.0 vs 1.0). Resolves GAP-1 from `docs/embed-integration.md`.
  **PT:** `UiLayerConfig::dp_ratio` (campo, padrão `1.0f`) e `UiLayer::set_dp_ratio(float)` (setter runtime): expõe `Rml::Context::SetDensityIndependentPixelRatio` para consumidores host. Quando definido, a unidade RCSS `dp` escala para `dp_ratio` pixels físicos (a unidade `px` não é afetada). Padrão recomendado: autore RCSS num tamanho lógico fixo usando unidades `dp`, chame `set_viewport(real_w, real_h)` e `set_dp_ratio(real_w / logical_w)`. Paridade `App`: `AppConfig::dp_ratio` e `App::set_dp_ratio(float)`. Verificado por `dp_ratio_sanity` (box branco 100dp×100dp; fator de escala 4.00x em ratio=2.0 vs 1.0). Resolve o GAP-1 de `docs/embed-integration.md`.
- **EN:** `UiLayer::set_asset_base_url(const char*)` and `App::set_asset_base_url(const char*)`: install a custom `Rml::FileInterface` (`src/base_url_file_interface.hpp`) that prepends a configurable base URL to relative asset paths. Enables documents to be loaded by relative name when their files live in a subdirectory or absolute root that differs from the process CWD. Absolute paths and paths already resolved by RmlUi are not affected. Call before `load()`. Pass `nullptr` or `""` to clear. Verified by `base_url_sanity` (renders `embed_scene.rml` exclusively from `base_url_assets/`; mean=58.5, dark=50%, bright=35%). Resolves GAP-3 from `docs/embed-integration.md`.
  **PT:** `UiLayer::set_asset_base_url(const char*)` e `App::set_asset_base_url(const char*)`: instalam um `Rml::FileInterface` customizado (`src/base_url_file_interface.hpp`) que prefixa um base URL configuravel a caminhos relativos de assets. Permite que documentos sejam carregados por nome relativo quando seus arquivos estao num subdiretorio ou raiz absoluta diferente do CWD do processo. Caminhos absolutos e paths ja resolvidos pelo RmlUi nao sao afetados. Chame antes de `load()`. Passe `nullptr` ou `""` para limpar. Verificado por `base_url_sanity` (renderiza `embed_scene.rml` exclusivamente de `base_url_assets/`; mean=58.5, dark=50%, bright=35%). Resolve o GAP-3 de `docs/embed-integration.md`.

---

## [0.2.1] - 2026-06-30

### Added / Adicionado

- **EN:** `GLINTFX_BACKEND_GLFW` CMake option (default `ON`): when set to `OFF`, the library is built in embed-only mode -- `glintfx::App`, `window_glfw.cpp`, and `RmlUi_Platform_GLFW.cpp` are excluded and `glfw` is not linked. Enables SDL3 / X11 host consumers (e.g. GusWorld) to link `glintfx::UiLayer` without dragging GLFW as a transitive dependency. `glintfxConfig.cmake` propagates the setting so `find_package(glintfx)` consumers are not forced to have GLFW installed in embed-only builds. The generated `<glintfx/config.hpp>` exposes `GLINTFX_BACKEND_GLFW` (0/1) for compile-time guards.
  **PT:** Opção CMake `GLINTFX_BACKEND_GLFW` (padrão `ON`): quando `OFF`, a biblioteca é compilada em modo embed-only -- `glintfx::App`, `window_glfw.cpp` e `RmlUi_Platform_GLFW.cpp` são excluídos e `glfw` não é linkado. Permite que consumidores host SDL3/X11 (ex.: GusWorld) linkem `glintfx::UiLayer` sem arrastar GLFW como dep transitiva. `glintfxConfig.cmake` propaga a opção para que consumidores `find_package(glintfx)` não sejam forçados a ter GLFW instalado em builds embed-only. O `<glintfx/config.hpp>` gerado expõe `GLINTFX_BACKEND_GLFW` (0/1) para guards em tempo de compilação.
- **EN:** CI -- embed-only build gate (`GLINTFX_BACKEND_GLFW=OFF`) added to the CI matrix: validates that the GLFW-free build path stays compilable and `UiLayer` tests pass on every push/PR.
  **PT:** CI -- gate de build embed-only (`GLINTFX_BACKEND_GLFW=OFF`) adicionado à matriz de CI: valida que o caminho de build sem GLFW permanece compilável e os testes do `UiLayer` passam em todo push/PR.

### Fixed / Corrigido

- **EN:** Embed-only build no longer forces GLFW as a transitive dependency of `glintfx::UiLayer` consumers. Prior to v0.2.1, `find_package(glfw3 REQUIRED)` was always called unconditionally, making any consumer of the library implicitly require GLFW even when only `UiLayer` was used.
  **PT:** Build embed-only não força mais GLFW como dep transitiva de consumidores de `glintfx::UiLayer`. Antes da v0.2.1, `find_package(glfw3 REQUIRED)` era sempre chamado incondicionalmente, fazendo qualquer consumidor da lib exigir implicitamente GLFW mesmo quando só `UiLayer` era usado.

---

## [0.2.0] - 2026-06-30

### Added / Adicionado

- **EN:** `glintfx::UiLayer` -- embed / guest mode: attaches the UI+effects engine to a host-owned GL context (no window created); compose-only render (no clear, no swap); GL state automatically saved and restored on return; neutral `UiEvent` input + gamepad navigation ([ADR-0008](docs/adr/0008-embed-guest-mode.md)). First target consumer: GusWorld.
  **PT:** `glintfx::UiLayer` -- embed / guest mode: anexa o motor de UI+efeitos ao contexto GL de um host (sem criar janela); render compose-only (sem clear, sem swap); estado GL salvo e restaurado automaticamente ao retornar; eventos neutros `UiEvent` + navegação por gamepad ([ADR-0008](docs/adr/0008-embed-guest-mode.md)). 1º consumidor-alvo: GusWorld.
- **EN:** `App::ok()`: distinguishes initialization failure from a window closed later; `running()` no longer conflates the two (L1.1-ERRSTRAT).
  **PT:** `App::ok()`: distingue falha de inicialização de janela fechada depois; `running()` não mistura mais os dois (L1.1-ERRSTRAT).
- **EN:** `find_package(glintfx)` support: `glintfxConfig.cmake` is installed alongside the library and headers; RmlUi is co-installed under the same prefix. Enables consumption from an installed tree without FetchContent (L1.8-FINDPKG).
  **PT:** Suporte a `find_package(glintfx)`: `glintfxConfig.cmake` instalado junto da lib e dos headers; RmlUi co-instalado sob o mesmo prefixo. Habilita consumo via árvore instalada sem FetchContent (L1.8-FINDPKG).
- **EN:** Versioned CI -- GitHub Actions (`.github/workflows/ci.yml`) and Codeberg Forgejo Actions (`.forgejo/workflows/ci.yml`) -- runs the full 5-test Xvfb suite on every push/PR (L1.2-CI).
  **PT:** CI versionado -- GitHub Actions (`.github/workflows/ci.yml`) e Codeberg Forgejo Actions (`.forgejo/workflows/ci.yml`) -- executa a suíte completa de 5 testes Xvfb em todo push/PR (L1.2-CI).

### Fixed / Corrigido

- **EN:** `App::snapshot()` PPM orientation: image was vertically flipped (OpenGL reads bottom-up); rows are now reversed before writing (L1.3-SNAPFLIP).
  **PT:** Orientação do PPM em `App::snapshot()`: imagem estava invertida verticalmente (OpenGL lê de baixo pra cima); linhas agora são revertidas antes de escrever (L1.3-SNAPFLIP).
- **EN:** Null / state guards in `window_glfw` and `App`: early returns on uninitialized state prevent silent UB when methods are called out of order or after a failed init (L1.4-GUARDS).
  **PT:** Guards de null/estado em `window_glfw` e `App`: retornos antecipados em estado não-inicializado evitam UB silencioso quando métodos são chamados fora de ordem ou após init falho (L1.4-GUARDS).
- **EN:** `body` fills the viewport in `showcase.rcss`: added `width: 100%; height: 100%;` so the dark background covers the full window (L1.5-DEMOBG).
  **PT:** `body` preenche o viewport em `showcase.rcss`: adicionado `width: 100%; height: 100%;` para o fundo escuro cobrir toda a janela (L1.5-DEMOBG).
- **EN:** `TESTES.md` -- corrected `xvfb-run` vs real GPU in visual validation and pixel-golden sections: headless/CI uses llvmpipe; real GPU validation runs without `xvfb-run` on the desktop DISPLAY (L1.6-TESTDOC).
  **PT:** `TESTES.md` -- corrigido `xvfb-run` vs GPU real nas seções de validação visual e pixel-golden: headless/CI usa llvmpipe; validação em GPU real roda sem `xvfb-run` no DISPLAY do desktop (L1.6-TESTDOC).

### Docs / Documentação

- **EN:** ADR-0008 -- embed / guest mode: documents the `UiLayer` architecture for attaching to a host-owned GL context (game / engine) without owning the window. First consumer: GusWorld / GusEngine (SDL3). See [`docs/adr/0008-embed-guest-mode.md`](docs/adr/0008-embed-guest-mode.md).
  **PT:** ADR-0008 -- embed / guest mode: documenta a arquitetura do `UiLayer` para anexar a um contexto GL de host (jogo / engine) sem ser dono da janela. Primeiro consumidor: GusWorld / GusEngine (SDL3). Ver [`docs/adr/0008-embed-guest-mode.md`](docs/adr/0008-embed-guest-mode.md).
- **EN:** v2 design spec: approved spec for the game UI component library (Atomic Design, tokens-first -- menus, dialogs, windows, fonts, effects). See [`docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`](docs/superpowers/specs/2026-06-30-glintfx-v2-design.md).
  **PT:** Spec de design da v2: spec aprovada da component library de UI de jogo (Atomic Design, tokens-first -- menus, diálogos, janelas, fontes, efeitos). Ver [`docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`](docs/superpowers/specs/2026-06-30-glintfx-v2-design.md).

---

## [0.1.0] - 2026-06-29

First release of **glintfx**: a drop-in C++ library for Linux x86-64 that fuses RmlUi 6.3 (HTML/CSS UI) with a GL3 effects renderer. / Primeiro lançamento do **glintfx**: biblioteca C++ drop-in para Linux x86-64 que funde RmlUi 6.3 (UI HTML/CSS) com um renderer de efeitos GL3.

### Added / Adicionado

- **EN:** Public RAII facade `glintfx::App` (move-only, pImpl) with `AppConfig` and `version()`; the only public header is `<glintfx/glintfx.hpp>` and it exposes no third-party types.
  **PT:** Fachada RAII pública `glintfx::App` (move-only, pImpl) com `AppConfig` e `version()`; o único header público é `<glintfx/glintfx.hpp>` e não expõe tipos de terceiros.
- **EN:** Four internal modules: M0 Bootstrap (RmlUi lifecycle), M1 Platform (GLFW window + FreeType font), M2 Render (wraps `RenderInterface_GL3`), M3 Facade.
  **PT:** Quatro módulos internos: M0 Bootstrap (ciclo de vida do RmlUi), M1 Platform (janela GLFW + fonte FreeType), M2 Render (embrulha o `RenderInterface_GL3`), M3 Facade.
- **EN:** Data-driven effects via RCSS: glow / box-shadow, drop-shadow, gradient, backdrop-blur, blur filter, and mask.
  **PT:** Efeitos data-driven via RCSS: glow / box-shadow, drop-shadow, degradê, backdrop-blur, filtro blur e mask.
- **EN:** Drop-in CMake consumption: single target `glintfx::glintfx` via FetchContent; RmlUi 6.3 fetched automatically, gl3w vendored (offline).
  **PT:** Consumo drop-in via CMake: alvo único `glintfx::glintfx` via FetchContent; RmlUi 6.3 baixado automaticamente, gl3w vendorizado (offline).
- **EN:** `glintfx_showcase` demo exercising all five effects, plus a `glintfx_capture` frame-capture tool (`App::snapshot`).
  **PT:** Demo `glintfx_showcase` exercitando os cinco efeitos, mais a ferramenta de captura `glintfx_capture` (`App::snapshot`).
- **EN:** Test suite under Xvfb: `window_smoke`, `render_smoke`, `bootstrap_smoke`, `app_smoke`, and `render_sanity`; opt-in pixel-exact `golden_test` (`-DGLINTFX_GOLDEN_TEST=ON`).
  **PT:** Suíte de testes sob Xvfb: `window_smoke`, `render_smoke`, `bootstrap_smoke`, `app_smoke` e `render_sanity`; `golden_test` pixel-exato opt-in (`-DGLINTFX_GOLDEN_TEST=ON`).
- **EN:** `consumer-example/` proving end-to-end drop-in consumption with no GL/GLFW/RmlUi references.
  **PT:** `consumer-example/` provando o consumo drop-in ponta a ponta sem referências a GL/GLFW/RmlUi.
- **EN:** Documentation: README, AGENTS.md, CONTRIBUTING.md, SECURITY.md, getting-started tutorial, and effects guide; `NOTICE` with third-party attributions.
  **PT:** Documentação: README, AGENTS.md, CONTRIBUTING.md, SECURITY.md, tutorial getting-started e guia de efeitos; `NOTICE` com atribuições de terceiros.

### Fixed / Corrigido

- **EN:** Opaque backbuffer fix for premultiplied alpha, preventing the compositor (Wayland/X) from showing a translucent window.
  **PT:** Fix de backbuffer opaco para alpha premultiplicado, evitando que o compositor (Wayland/X) mostre a janela translúcida.
- **EN:** MSAA disabled on the render layer (`RMLUI_NUM_MSAA_SAMPLES=0`) to avoid a black postprocess texture under Mesa/llvmpipe.
  **PT:** MSAA desabilitado no render layer (`RMLUI_NUM_MSAA_SAMPLES=0`) para evitar textura de pós-processo preta sob Mesa/llvmpipe.

### Changed / Mudado

- **EN:** Project relicensed from AGPL-3.0-or-later to **MPL-2.0** ([ADR-0007](docs/adr/0007-license-mpl-2.0.md)) to enable external adoption.
  **PT:** Projeto relicenciado de AGPL-3.0-or-later para **MPL-2.0** ([ADR-0007](docs/adr/0007-license-mpl-2.0.md)) para viabilizar adoção externa.

### Known limitations / Limitações conhecidas

- **EN:** Linux x86-64 only; one `App` per process; the `mask` effect requires a real GPU (crashes under Mesa/llvmpipe); GLFW only (SDL/X11 not implemented); no versioned CI yet; consumption via FetchContent / `add_subdirectory` only (full `find_package` is post-v1).
  **PT:** Apenas Linux x86-64; um `App` por processo; o efeito `mask` exige GPU real (crasha sob Mesa/llvmpipe); apenas GLFW (SDL/X11 não implementados); sem CI versionado ainda; consumo só via FetchContent / `add_subdirectory` (`find_package` completo é pós-v1).

> **Note / Nota:** Layer 0 (`loucura_c_asm`, the pure C/ASM runtime) is not part of this release; it is an independent experimental track with its own future tag. / A Camada 0 (`loucura_c_asm`, o runtime C/ASM puro) não faz parte deste lançamento; é uma trilha experimental independente com tag futura própria.

[0.1.0]: https://codeberg.org/petrinhu/glintfx/releases/tag/v0.1.0
