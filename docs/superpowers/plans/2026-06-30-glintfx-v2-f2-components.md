# glintfx v2 · F2 — component library (tokens + componentes onda 1) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Entregar a **F2** do spec v2: uma **component library temável e composável**, data-driven, distribuída por um alvo CMake **opt-in `glintfx::ui`** (aditivo, não toca o engine `glintfx::glintfx`). Inclui `tokens.rcss` (2 níveis), `glintfx.rcss` (componentes + modificadores de efeito ortogonais) e templates RML. 1ª onda **amarrada às telas reais do GusWorld** (cockpit/HUD, diálogo narrativo, menu): **button, label, panel, dialog, menu**. Funciona nos **dois modos** (`glintfx::App` standalone e `glintfx::UiLayer` embed). Theming (default escuro + high-contrast + reduced-motion), foco/gamepad e contraste AA.

**Architecture:** Componentes = **dados** (`.rcss` + templates `.rml`), zero C++ por componente (regra do spec §6). O único C++ novo é a **API de tema** (`set_theme`) no engine, exposta por `App` e `UiLayer` e encaminhada ao `Rml::Context::ActivateTheme` (capacidade de engine, não por-componente). Os efeitos são **modificadores ortogonais** (`.glow/.glass/.gradient` → N+M regras, não N×M; `.masked` só GPU real). Tokens semânticos são redefinidos por `@media (theme: <nome>)`; os componentes consomem só os semânticos via `var()`.

**Tech Stack:** C++ (CMake ≥3.16), RmlUi @`2cd28864ae25ed345b70598751703a5433b12356`, gl3w vendorizado, GLFW (modo App/harness), OpenGL 3.3 core, FreeType. Testes sob Xvfb. Componentes em RCSS/RML (sintaxe RmlUi 6.3).

**Achados de verificação (confirmados no fonte pinado `glintfx/build/_deps/rmlui-src`):**
- **`var()` + custom properties: SUPORTADO** (`Source/Core/ElementStyle.cpp:165` resolve `var(`; há infra de "custom property"). → tokens-first é viável.
- **`@media (theme: <string>)`: NATIVO** — `enum MediaQueryId::Theme`, registrado com parser `string` (`Source/Core/StyleSheetParser.cpp:264`); casa quando `context->IsThemeActive(<nome>)` (`Source/Core/StyleSheetContainer.cpp:106`).
- **Ativar tema:** `Rml::Context::ActivateTheme(const String& name, bool activate)` + `IsThemeActive` (`Include/RmlUi/Core/Context.h:96`). → a API `set_theme` encaminha pra cá.
- **Sintaxe RCSS 6.3 (do `demos/showcase/showcase.rcss`):** `box-shadow: COLOR x y blur spread` (cor **primeiro**); `filter: drop-shadow(COLOR x y blur)` (cor primeiro); gradiente via `decorator: linear-gradient(...)` (não `background:`); `mask` via `decorator`; cor hex **8 dígitos `#rrggbbaa`**; `display: block` explícito (default do RmlUi é `inline`); `@font-face` obrigatório.

**Referências:** spec v2 §5–9, §11–12 (`../specs/2026-06-30-glintfx-v2-design.md`); F1 (`./2026-06-30-glintfx-v2-f1-embed-mode.md`); código real lido: `glintfx/CMakeLists.txt`, `glintfx/include/glintfx/{app,ui_layer,ui_event}.hpp`, `glintfx/demos/showcase/{showcase.rcss,showcase.rml,CMakeLists.txt}`, `glintfx/tests/CMakeLists.txt`.

## Global Constraints

(Cada task herda implicitamente esta seção.)

- **Plataforma:** Linux x86-64 apenas. **Padrão C++:** alvo `cxx_std_23`, piso C++17.
- **Licença:** `SPDX-License-Identifier: MPL-2.0` no topo de todo arquivo novo (`.hpp`/`.cpp`/`CMakeLists.txt` com `#`; `.rcss`/`.rml` com comentário `/* ... */` / `<!-- ... -->`). © 2026 Petrus Silva Costa.
- **Idioma:** identificadores/classes **en-intl**; comentários/doc-comments **bilíngues** (en, depois pt).
- **Encapsulamento (gate por grep):** nenhum tipo de RmlUi/GLFW/GL/gl3w/SDL em `include/glintfx/*.hpp`. A API de tema usa só tipos neutros (`const char*`, `bool`).
- **Invariante de não-regressão:** v1/v0.2 intactos — `glintfx::glintfx` (engine), `App`, `UiLayer` e os **10 testes** atuais seguem **verdes** sem mudar de expectativa. `glintfx::ui` é **aditivo**: não altera o consumo de quem usa só o engine.
- **Data-driven:** **nenhum C++ novo por componente** (componente = `.rcss` + opcional template `.rml`). Único C++ novo permitido na F2 = `set_theme` (engine) + helpers de teste.
- **Efeitos:** `.glow/.glass/.gradient` 1ª classe (paramétricos por token). **`.masked` é GPU-only** — fora do CI por software (llvmpipe crasha no BlendMask dual-sampler, bug do Mesa SW, já documentado); só sob `option(GLINTFX_GPU_TESTS OFF)`.
- **Tokens:** consumidor/componentes usam só os **semânticos**; primitivos não aparecem em componente. Cor `#rrggbbaa`.
- **Anti-OE / YAGNI:** só os componentes da 1ª onda do spec (button, label, panel, dialog, menu). **Sem** field/input/badge/icon a menos que uma tela real do GusWorld exija (registrar como onda 2, não implementar). Sem editor de tema, sem múltiplos temas além de default+high-contrast+reduced-motion.
- **a11y (limite honesto):** foco visível, nav teclado/gamepad, contraste AA (texto ≥4.5:1, UI/foco ≥3:1), high-contrast, reduced-motion, cor nunca sozinha. **Sem** promessa de screen reader/ARIA (RmlUi não tem).
- **Testes de GL:** sob `xvfb-run` via `tests/run_xvfb.cmake`; gates de componente no estilo `render_sanity` (estatística agregada, determinístico).

---

## File Structure

```
glintfx/
  CMakeLists.txt                       # + alvo glintfx_ui (INTERFACE) + alias glintfx::ui + install de ui/
  include/glintfx/
    app.hpp                            # + set_theme(const char*, bool)
    ui_layer.hpp                       # + set_theme(const char*, bool)
  src/
    app.cpp / ui_layer.cpp             # set_theme → engine.set_theme
    engine.hpp / engine.cpp            # + set_theme → context->ActivateTheme
  ui/                    [NOVO]        # os ATIVOS da component library (dados)
    tokens.rcss                        # T1: primitivos→semânticos + @media theme/reduced-motion
    glintfx.rcss                       # T2–T4: componentes + modificadores de efeito
    templates/
      dialog.rml                       # T3: template do diálogo narrativo (retrato+nome+texto+avançar)
      menu.rml                         # T3: template de lista de menu navegável
    assets/                            # fonte default (reusa a do showcase) + LICENSE
  demos/components/      [NOVO]        # T6: showcase dos componentes
    components.rml / components.rcss
    main.cpp / CMakeLists.txt
  tests/
    tokens_resolve.cpp  [NOVO]         # T1: var()/token resolve no tema default
    button_label.cpp    [NOVO]         # T2
    panel_dialog_menu.cpp [NOVO]       # T3
    effect_modifiers.cpp[NOVO]         # T4: N+M (componente + modificador)
    theme_switch.cpp    [NOVO]         # T5: ActivateTheme muda token
    contrast_audit.cpp  [NOVO]         # T5: razão WCAG dos tokens semânticos
    components_sanity.cpp [NOVO]       # T6: showcase nos 2 modos (App + UiLayer/FBO)
    ui/ (copiado no build)             # tokens.rcss/glintfx.rcss/templates copiados p/ os testes
    CMakeLists.txt                     # registra os novos testes (run_xvfb.cmake)
```

Mapeamento spec v2: F2 = §5 (tokens) + §6 (componentes onda 1) + §7 (modificadores) + §8 (theming/gamepad/a11y) + §9 (`glintfx::ui`). F3 (adoção GusWorld) e F4 (portfólio) ficam fora.

---

### Task 1: Alvo `glintfx::ui` + `tokens.rcss` (2 níveis) + teste de resolução de token

**Files:**
- Create: `glintfx/ui/tokens.rcss`, `glintfx/ui/glintfx.rcss` (vazio com SPDX por ora), `glintfx/ui/assets/` (copiar/symlink da fonte do showcase), `glintfx/tests/tokens_resolve.cpp`, `glintfx/tests/ui/min_tokens.rml`
- Modify: `glintfx/CMakeLists.txt` (alvo `glintfx_ui` INTERFACE + alias + install), `glintfx/tests/CMakeLists.txt`

**Interfaces:**
- Alvo CMake `glintfx::ui` (INTERFACE, aditivo) que expõe a pasta de ativos via a var de cache `GLINTFX_UI_ASSET_DIR` e instala `ui/` em `${CMAKE_INSTALL_DATADIR}/glintfx/ui`.
- `tokens.rcss`: nível 1 (primitivos `--c-*`) + nível 2 (semânticos `--color-*`, `--space-*`, `--radius-*`, `--elev-*`, `--motion*`, params de efeito `--glow-*`/`--blur-radius`/`--grad-*`) definidos no `body`; default = tema escuro; `@media (theme: high-contrast)` e `@media (theme: reduced-motion)` redefinem semânticos.

- [ ] **Step 1: `tokens.rcss` (primitivos→semânticos + media themes)**
```css
/* SPDX-License-Identifier: MPL-2.0
   EN: glintfx design tokens — level 1 (primitives) → level 2 (semantics). Components use ONLY
       semantics via var(). Default theme = dark; variants via @media (theme: ...).
   PT: Tokens de design do glintfx — nível 1 (primitivos) → nível 2 (semânticos). Componentes usam
       SÓ semânticos via var(). Tema default = escuro; variantes via @media (theme: ...).
   NOTE (RmlUi 6.3): hex is #rrggbbaa; box-shadow color-FIRST; gradients via decorator:. */
body {
  /* level 1 — primitives */
  --c-ink-900:#0b0e14ff; --c-ink-800:#14171fff; --c-ink-700:#1c2230ff;
  --c-paper-50:#f5f7faff; --c-paper-300:#c3ccdaff;
  --c-cyan-400:#5fd0ffff; --c-red-500:#e02828ff; --c-amber-300:#ffee00ff;
  /* level 2 — semantics (default = DARK, baked in) */
  --color-bg:var(--c-ink-900); --color-surface:var(--c-ink-800);
  --color-surface-hover:var(--c-ink-700); --color-surface-active:var(--c-ink-700);
  --color-text:var(--c-paper-50); --color-text-muted:var(--c-paper-300);
  --color-accent:var(--c-cyan-400); --color-focus:var(--c-cyan-400); --color-danger:var(--c-red-500);
  /* spacing 4/8/16/24/40 */
  --space-1:4px; --space-2:8px; --space-3:16px; --space-4:24px; --space-5:40px;
  /* radius */ --radius-sm:4px; --radius-md:8px; --radius-lg:16px; --radius-pill:999px;
  /* elevation (string, color-FIRST) */ --elev-1:#00000066 0px 2px 6px 0px; --elev-2:#00000088 0px 6px 18px 0px;
  /* motion */ --motion-fast:0.12s; --motion-base:0.20s; --motion:var(--motion-base);
  /* effect params (parametric) */
  --glow-color:var(--c-cyan-400); --glow-blur:16px; --glow-spread:2px;
  --blur-radius:8px; --grad-a:#6a0ddcff; --grad-b:#0f8a4aff; --grad-angle:135deg;
}
@media (theme: high-contrast) {
  body { --color-bg:#000000ff; --color-surface:#000000ff; --color-text:#ffffffff;
         --color-text-muted:#ffffffff; --color-focus:var(--c-amber-300); }
}
@media (theme: reduced-motion) { body { --motion:0s; } }
```
> `glintfx.rcss` nasce só com o cabeçalho SPDX + `@font-face` (componentes entram em T2–T4). `ui/assets/` reusa a fonte `OpenSans-Regular.ttf` do showcase (copiar o arquivo + a `LICENSE`).

- [ ] **Step 2: Alvo `glintfx_ui` (INTERFACE, aditivo) no CMake**

Em `glintfx/CMakeLists.txt`, **depois** do alias `glintfx::glintfx` e **sem** tocar o alvo `glintfx`:
```cmake
# EN: glintfx::ui — additive, asset-only component library (tokens.rcss + glintfx.rcss + templates).
#     INTERFACE target: carries no compiled code (components are data); exposes the asset dir.
# PT: glintfx::ui — biblioteca de componentes aditiva, só-ativos (tokens.rcss + glintfx.rcss + templates).
#     Alvo INTERFACE: sem código compilado (componentes são dados); expõe o diretório de ativos.
add_library(glintfx_ui INTERFACE)
add_library(glintfx::ui ALIAS glintfx_ui)
set(GLINTFX_UI_ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ui CACHE PATH "glintfx UI assets (rcss/rml)")
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  install(DIRECTORY ui/ DESTINATION ${CMAKE_INSTALL_DATADIR}/glintfx/ui)
endif()
```
> Anti-OE: nenhum `target_sources`/código em `glintfx_ui`. A API de tema (C++) vai no engine `glintfx` (T5), não aqui.

- [ ] **Step 3: Teste de resolução de token (falha primeiro — assets ausentes)**

`glintfx/tests/ui/min_tokens.rml` — um body que usa um token semântico:
```html
<!-- SPDX-License-Identifier: MPL-2.0 -->
<rml><head>
  <link type="text/rcss" href="ui/tokens.rcss"/>
  <style> body { display:block; width:100%; height:100%; background-color: var(--color-bg); } </style>
</head><body></body></rml>
```
`glintfx/tests/tokens_resolve.cpp`: abre `App` 200×120, `load("tests/ui/min_tokens.rml")`, snapshot, lê o pixel central e confere que bate (aprox.) com `--color-bg` do tema default (`#0b0e14` → R≈11,G≈14,B≈20, tolerância ±6). Prova que `var()` resolve o token.
No `tests/CMakeLists.txt`: `file(COPY ${GLINTFX_UI_ASSET_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})` (vira `build/tests/ui/`), copiar `min_tokens.rml` p/ `build/tests/ui/`, registrar `tokens_resolve` (link `glintfx`, `run_xvfb.cmake`, `WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}`).

- [ ] **Step 4: Build + rodar (passa) + commit**
Run: `cd glintfx && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=OFF && cmake --build build -j && ctest --test-dir build -R tokens_resolve --output-on-failure`
Expected: `tokens_resolve: PASS` (pixel central ≈ `--color-bg`).
```bash
git add glintfx/ && git commit -m "feat(glintfx-ui): alvo opt-in glintfx::ui + tokens.rcss 2 níveis (F2 T1)"
```

---

### Task 2: Componentes atoms — `button` + `label` (estados + foco)

**Files:**
- Modify: `glintfx/ui/glintfx.rcss` (regras de `.gfx-button` e `.gfx-label`)
- Create: `glintfx/tests/button_label.cpp`, `glintfx/tests/ui/button_label.rml`
- Modify: `glintfx/tests/CMakeLists.txt`

**Interfaces (classes RCSS — contrato estável entre tarefas):** `.gfx-button` (use em `<button class="gfx-button">`), `.gfx-label` (`<span class="gfx-label">`), `.gfx-label--muted`. Estados nativos: `:hover`, `:active`, `:focus` (anel de foco com `--color-focus`).

- [ ] **Step 1: Regras de `button`/`label` em `glintfx.rcss`**
```css
/* button — atom; preferir <button> nativo (foco/teclado de graça). */
.gfx-button {
  display: inline-block;
  padding: var(--space-2) var(--space-3);
  border-radius: var(--radius-md);
  background-color: var(--color-surface);
  color: var(--color-text);
  font-family: Sans; font-size: 16px;
}
.gfx-button:hover  { background-color: var(--color-surface-hover); }
.gfx-button:active { background-color: var(--color-surface-active); }
/* foco visível (a11y): anel com a cor de foco — box-shadow é color-FIRST no RmlUi */
.gfx-button:focus  { box-shadow: var(--color-focus) 0px 0px 0px 2px; }
/* label — atom */
.gfx-label        { color: var(--color-text); font-family: Sans; font-size: 16px; }
.gfx-label--muted { color: var(--color-text-muted); }
```
> Confirmar no fonte pinado que `<button>` é registrado e aceita `:focus`/`:hover` no RmlUi 6.3 (`Source/Core/Elements/ElementFormControl*`); ajustar o seletor se o elemento nativo diferir.

- [ ] **Step 2: Cena de teste + teste (falha primeiro)**

`tests/ui/button_label.rml`: linka `ui/tokens.rcss` + `ui/glintfx.rcss`; body escuro com um `<button class="gfx-button">OK</button>` e um `<span class="gfx-label">HP</span>` + `<span class="gfx-label gfx-label--muted">10/10</span>`.
`tests/button_label.cpp`: `App` 320×160, load, 2 frames warm-up, snapshot, estatística agregada estilo `render_sanity`: não-preto (`mean>5`), há fundo escuro (`dark≥10%`), há conteúdo claro do texto/superfície (`bright≥0.3%`). Imprime `button_label: PASS`.

- [ ] **Step 3: Build + rodar + commit**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R button_label --output-on-failure`
Expected: `button_label: PASS`.
```bash
git add glintfx/ && git commit -m "feat(glintfx-ui): componentes button + label (estados/foco) (F2 T2)"
```

---

### Task 3: Componentes organisms — `panel` + `dialog` + `menu` (+ templates RML)

**Files:**
- Modify: `glintfx/ui/glintfx.rcss` (`.gfx-panel`, `.gfx-dialog*`, `.gfx-menu*`)
- Create: `glintfx/ui/templates/dialog.rml`, `glintfx/ui/templates/menu.rml`, `glintfx/tests/panel_dialog_menu.cpp`, `glintfx/tests/ui/pdm.rml`
- Modify: `glintfx/tests/CMakeLists.txt`

**Interfaces (classes RCSS):** `.gfx-panel`; diálogo `.gfx-dialog` > `.gfx-dialog__portrait` / `.gfx-dialog__name` / `.gfx-dialog__text` / `.gfx-dialog__next`; menu `.gfx-menu` > `.gfx-menu__item` (navegável, `:focus`).

- [ ] **Step 1: Regras `panel`/`dialog`/`menu` em `glintfx.rcss`**
```css
/* panel — organism: moldura do cockpit/HUD */
.gfx-panel {
  display: block; padding: var(--space-4);
  border-radius: var(--radius-lg);
  background-color: var(--color-surface);
  box-shadow: var(--elev-1);            /* elevação como string (color-first) */
}
/* dialog — caixa de diálogo narrativa do GusWorld */
.gfx-dialog { display: block; width: 600px; padding: var(--space-3);
  border-radius: var(--radius-lg); background-color: var(--color-surface); box-shadow: var(--elev-2); }
.gfx-dialog__portrait { display: inline-block; width: 64px; height: 64px; border-radius: var(--radius-md); }
.gfx-dialog__name { display: block; color: var(--color-accent); font-size: 18px; }
.gfx-dialog__text { display: block; color: var(--color-text); font-size: 16px; }
.gfx-dialog__next { display: block; color: var(--color-text-muted); font-size: 14px; }
/* menu — lista navegável por teclado/gamepad */
.gfx-menu { display: block; }
.gfx-menu__item { display: block; padding: var(--space-2) var(--space-3); color: var(--color-text);
  border-radius: var(--radius-md); }
.gfx-menu__item:hover { background-color: var(--color-surface-hover); }
.gfx-menu__item:focus { box-shadow: var(--color-focus) 0px 0px 0px 2px; }  /* foco direcional (gamepad) */
```

- [ ] **Step 2: Templates RML reutilizáveis**

`ui/templates/dialog.rml` (estrutura do diálogo — retrato+nome+texto+avançar) e `ui/templates/menu.rml` (lista de `.gfx-menu__item` com `tabindex` para foco direcional). Marcar os itens do menu como focáveis (atributo de foco do RmlUi: `tabindex="auto"`/elemento nativo) para a nav por gamepad funcionar.
> Confirmar no fonte pinado o atributo correto de foco navegável do RmlUi 6.3 (`tabindex`/`nav`) — `Source/Core/Elements` / docs; ajustar.

- [ ] **Step 3: Cena + teste (falha primeiro)**

`tests/ui/pdm.rml`: linka tokens+glintfx.rcss; um `.gfx-panel` contendo um `.gfx-dialog` (com nome+texto) e um `.gfx-menu` com 3 `.gfx-menu__item`. `tests/panel_dialog_menu.cpp`: `App` 700×500, load, warm-up, snapshot, estatística agregada (não-preto, fundo escuro, conteúdo claro). Adicionar **um cheque de foco**: injetar foco no 1º item e verificar que renderiza o anel (via `bright`/cor de foco presente acima do baseline) — ou, se difícil de isolar visualmente, registrar como cheque de nav no T5. Imprime `panel_dialog_menu: PASS`.

- [ ] **Step 4: Build + rodar + commit**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R panel_dialog_menu --output-on-failure`
Expected: `panel_dialog_menu: PASS`.
```bash
git add glintfx/ && git commit -m "feat(glintfx-ui): componentes panel + dialog + menu + templates RML (F2 T3)"
```

---

### Task 4: Efeitos como modificadores ortogonais (`.glow/.glass/.gradient`; `.masked` GPU-only)

**Files:**
- Modify: `glintfx/ui/glintfx.rcss` (modificadores)
- Create: `glintfx/tests/effect_modifiers.cpp`, `glintfx/tests/ui/effects.rml`
- Modify: `glintfx/tests/CMakeLists.txt` (registrar teste; `.masked` atrás de `GLINTFX_GPU_TESTS`)

**Interfaces (classes RCSS modificadoras, ortogonais a qualquer componente):** `.glow`, `.glass`, `.gradient`, `.masked`. Combinam no markup: `<div class="gfx-panel glass gradient">`.

- [ ] **Step 1: Modificadores paramétricos em `glintfx.rcss`**
```css
/* efeitos = modificadores ortogonais (N+M, não N×M); paramétricos via tokens de efeito */
.glow     { box-shadow: var(--glow-color) 0px 0px var(--glow-blur) var(--glow-spread); }
.gradient { decorator: linear-gradient(var(--grad-angle), var(--grad-a), var(--grad-b)); }
.glass    { backdrop-filter: blur(var(--blur-radius)); background-color: #ffffff14; }
/* .masked: SÓ GPU real — crasha no llvmpipe (BlendMask dual-sampler). NÃO usar no CI por software. */
.masked   { mask-image: linear-gradient(90deg, #000000ff, #00000000); }
```
> Confirmar que `var()` é aceito **dentro** de `linear-gradient(...)`/`blur(...)` no RmlUi 6.3 (substituição textual antes do parse — provável OK; se não, materializar os valores no componente e expor menos parametrização). `backdrop-filter`/`mask-image` confirmados pelo showcase.

- [ ] **Step 2: Teste N+M (componente + modificador) — falha primeiro**

`tests/ui/effects.rml`: dois `.gfx-panel` — um `.gfx-panel.glow` (sobre fundo escuro) e um `.gfx-panel.gradient` — **sem** `.masked`. `tests/effect_modifiers.cpp`: `App` 500×400, load, warm-up, snapshot, estatística: `bright≥1%` (glow ciano + gradiente colorido presentes), fundo escuro presente. Prova que o mesmo componente ganha efeito por classe modificadora, sem regra dedicada. Imprime `effect_modifiers: PASS`.

- [ ] **Step 3: `.masked` documentado e GPU-gated (NÃO no CI por software)**

No `tests/CMakeLists.txt`, espelhar o padrão `GLINTFX_GOLDEN_TEST`:
```cmake
option(GLINTFX_GPU_TESTS "Enable effect tests that require a real GPU (e.g. .masked)" OFF)
if(GLINTFX_GPU_TESTS)
  add_executable(masked_effect masked_effect.cpp)
  target_link_libraries(masked_effect PRIVATE glintfx)
  add_test(NAME masked_effect COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:masked_effect> -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()
```
(O `masked_effect.cpp`/`ui/masked.rml` podem ser criados como stub mínimo aqui; só rodam com `-DGLINTFX_GPU_TESTS=ON` em hardware real.)

- [ ] **Step 4: Build + rodar (sem GPU) + commit**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R effect_modifiers --output-on-failure`
Expected: `effect_modifiers: PASS`. `masked_effect` **não** aparece (gated OFF).
```bash
git add glintfx/ && git commit -m "feat(glintfx-ui): efeitos modificadores ortogonais glow/glass/gradient; masked GPU-only (F2 T4)"
```

---

### Task 5: Theming (`set_theme`) + a11y (foco/gamepad/contraste AA)

**Files:**
- Modify: `glintfx/include/glintfx/app.hpp`, `glintfx/include/glintfx/ui_layer.hpp` (`set_theme`), `glintfx/src/app.cpp`, `glintfx/src/ui_layer.cpp`, `glintfx/src/engine.hpp`, `glintfx/src/engine.cpp`
- Create: `glintfx/tests/theme_switch.cpp`, `glintfx/tests/contrast_audit.cpp`, `glintfx/tests/ui/theme_probe.rml`
- Modify: `glintfx/tests/CMakeLists.txt`

**Interfaces:**
- `void App::set_theme(const char* name, bool on)` e `void UiLayer::set_theme(const char* name, bool on)` (neutros) → `Engine::set_theme` → `context->ActivateTheme(name, on)`.
- Nav por gamepad: já viável via F1 `process_event` (`UiEvent::Key::Up/Down/...`) → foco direcional do RmlUi; os componentes `menu`/`dialog` desta F2 são foco-first.

- [ ] **Step 1: `Engine::set_theme` + fachadas**

`engine.hpp`: `void set_theme(const char* name, bool on);` `engine.cpp`: `if (auto* c = impl_->boot.context()) c->ActivateTheme(name, on);`. `App`/`UiLayer`: método público `set_theme` que encaminha (guard `ok()`), **sem** vazar tipos. Atualizar o umbrella/doc-comments (bilíngue).

- [ ] **Step 2: Teste de troca de tema (falha primeiro)**

`tests/ui/theme_probe.rml`: body com `background-color: var(--color-bg)` (igual ao min_tokens). `tests/theme_switch.cpp`: `App` 200×120, load; snapshot do tema default (pixel ≈ `#0b0e14`); `app.set_theme("high-contrast", true)`; `update()`+`render()`; snapshot 2 (pixel ≈ `#000000`). Asserção: o pixel central **mudou** de `#0b0e14` para `#000000` (prova que `@media (theme: high-contrast)` remapeou `--color-bg`). Imprime `theme_switch: PASS`.

- [ ] **Step 3: Auditoria de contraste (WCAG AA) sobre os tokens**

`tests/contrast_audit.cpp`: lê `tests/ui/tokens.rcss` (copiado no build), extrai por regex os hex de `--color-text`/`--color-bg`/`--color-focus`/`--color-surface` no bloco default e no bloco `@media (theme: high-contrast)`, resolve `var()` de 1 nível (primitivo→hex), calcula a **razão de contraste WCAG** (luminância relativa) e falha se: texto/bg **< 4.5:1** ou foco/bg **< 3:1**, em cada tema. Imprime as razões e `contrast_audit: PASS`.
> Nota anti-flaky: o teste calcula em CPU (sem GPU); é determinístico. Mantém a paleta honesta a cada mudança de token.

- [ ] **Step 4: Build + rodar + gate de header + commit**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R 'theme_switch|contrast_audit' --output-on-failure`
Expected: ambos `PASS`.
Run: `! grep -rqE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/ && echo OK`
```bash
git add glintfx/ && git commit -m "feat(glintfx): set_theme (ActivateTheme) + auditoria de contraste AA (F2 T5)"
```

---

### Task 6: Showcase de componentes + gate de sanidade nos DOIS modos

**Files:**
- Create: `glintfx/demos/components/{components.rml,components.rcss,main.cpp,CMakeLists.txt}`, `glintfx/tests/components_sanity.cpp`
- Modify: `glintfx/CMakeLists.txt` (add `demos/components` quando `GLINTFX_BUILD_DEMOS`), `glintfx/tests/CMakeLists.txt`

**Interfaces:** nenhuma nova — consome só as classes RCSS das T2–T4 + tokens (T1) + `set_theme` (T5).

- [ ] **Step 1: Demo `components` (uma tela representativa do GusWorld)**

`demos/components/components.rml`: linka `ui/tokens.rcss` + `ui/glintfx.rcss`; monta um mini-cockpit: um `.gfx-panel.gradient` com `.gfx-label` (HP/AP), um `.gfx-dialog` (nome+texto narrativo), um `.gfx-menu` com 3 `.gfx-menu__item` e um `.gfx-button.glow`. `main.cpp` = `App` + `load` + `run` (espelha `demos/showcase/main.cpp`). `CMakeLists.txt` copia `${GLINTFX_UI_ASSET_DIR}` + `components.*` pro binário. Linka `glintfx` + (assets de) `glintfx::ui`.

- [ ] **Step 2: Gate de sanidade nos 2 modos**

`tests/components_sanity.cpp` faz a MESMA cena render-sanity em dois caminhos:
1. **App standalone:** `App` 900×600, load `components.rml`, snapshot → estatística agregada (não-preto, fundo escuro ≥10%, conteúdo claro ≥1%).
2. **UiLayer embed (offscreen):** reusa o harness `tests/offscreen.hpp` da F1 — host cria contexto (janela oculta) + FBO 900×600, **host limpa** o FBO, `UiLayer` attach + load `components.rml` + `render()` compose-only + readback → mesma estatística + **fundo do host preservado** onde a UI é transparente.
Falha se qualquer modo não bater. Imprime `components_sanity: PASS (App+UiLayer)`.
> Lembrete N3 (F1): `App` e `UiLayer` são processo-global do RmlUi — **não** instanciar os dois vivos ao mesmo tempo. Rodar o caminho App, destruí-lo, **depois** o caminho UiLayer (ou dois processos). Estruturar o teste assim.

- [ ] **Step 3: Build + rodar + commit**
Run: `cd glintfx && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=ON && cmake --build build -j && ctest --test-dir build -R components_sanity --output-on-failure`
Expected: `components_sanity: PASS (App+UiLayer)`.
```bash
git add glintfx/ && git commit -m "feat(glintfx-ui): showcase de componentes + gate de sanidade App+UiLayer (F2 T6)"
```

---

### Task 7: Integração final — build limpo, suíte completa, gates e fechamento

**Files:**
- Modify: `docs/superpowers/plans/2026-06-30-glintfx-v2-f2-components.md` (marcar checkboxes); `README.md`/`CHANGELOG.md` (entrada da F2 — opcional, recomendado)

- [ ] **Step 1: Build do zero + suíte inteira verde**
Run: `cd glintfx && rm -rf build && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=ON && cmake --build build -j 2>&1 | tail -8`
Run: `ctest --test-dir build --output-on-failure`
Expected: os **10** testes de v1/v0.2 (`window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke`, `render_sanity`, `ui_layer_attach`, `ui_layer_compose`, `gl_state_guard`, `ui_layer_events`, `ui_layer_sanity`) seguem verdes **+** os **7** novos da F2 (`tokens_resolve`, `button_label`, `panel_dialog_menu`, `effect_modifiers`, `theme_switch`, `contrast_audit`, `components_sanity`). `masked_effect` ausente (GPU-gated OFF).

- [ ] **Step 2: Gates finais (engine intacto + encapsulamento + SPDX)**
Run: `git diff --stat v0.2.0 -- glintfx/src/render_gl3.cpp glintfx/src/window_glfw.cpp` — esperado: **vazio** (engine de render/janela não muda na F2; só `app.cpp`/`ui_layer.cpp`/`engine.*` ganham `set_theme`).
Run: `! grep -rqE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/ && echo "headers públicos limpos"`
Run: `for f in $(git ls-files 'glintfx/ui/**' 'glintfx/demos/components/**' 'glintfx/tests/**' | grep -vE '_deps|build/'); do head -1 "$f" | grep -q 'SPDX-License-Identifier: MPL-2.0' || echo "SEM SPDX: $f"; done` (esperado: sem saída)

- [ ] **Step 3: Prova aditiva do `glintfx::ui`**
Confirmar que um consumidor que linka **só** `glintfx::glintfx` (engine) continua buildando sem os ativos de `ui/` (o alvo `glintfx::ui` é opt-in). Cobrir pelo fato de os testes de engine (T1 da F1) não dependerem de `ui/`.

- [ ] **Step 4: Fechar o plano**
Marcar todos os checkboxes; conferir o spec §12-F2 satisfeito: tokens 2 níveis + componentes onda 1 + modificadores + theming (default/high-contrast/reduced-motion) + contraste AA, nos dois modos. Próximo: **F3** (adoção no GusWorld via `UiLayer` + `glintfx::ui`).

---

## Notas de execução

- **Ordem de risco:** T1 valida a fundação (`var()` resolve token no tema default) — se falhar, todo o resto cai; T5 valida o theming real (`ActivateTheme` muda pixel) e a honestidade da paleta (contraste). T6 prova os componentes nos **dois modos** (o requisito transversal da v2).
- **`.masked`:** nunca no CI por software (llvmpipe crasha). Só sob `-DGLINTFX_GPU_TESTS=ON` em hardware real. Documentado no `.rcss` e no `tests/CMakeLists.txt`.
- **RmlUi global (N3):** `App` e `UiLayer` compartilham o estado global do RmlUi — nunca vivos ao mesmo tempo (relevante no `components_sanity`).
- **Pontos a confirmar no fonte pinado** (ajustar se divergirem): `<button>` nativo + `:focus`/`:hover`; atributo de foco navegável (`tabindex`/`nav`) para a nav por gamepad; aceitação de `var()` **dentro** de `linear-gradient(...)`/`blur(...)`. Os achados de verificação no cabeçalho cobrem `var()`, `@media (theme:)`, `ActivateTheme` e a sintaxe de efeitos.
- **Anti-OE:** F2 entrega só button/label/panel/dialog/menu + glow/glass/gradient + 1 tema default + high-contrast + reduced-motion. `field`/`input`/`badge`/`icon`/`card`/`navbar`/`modal` e temas extras = **onda 2/F4**, só com caso de uso real do GusWorld (§6, §13 do spec).
- **F3 (fora deste plano):** GusWorld migra cockpit/HUD, diálogo e menu para `UiLayer` + `glintfx::ui`, aposentando o backend vendorizado (ADR-010 deles).
