# Changelog

> **EN:** All notable changes to glintfx. Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); this project follows [Semantic Versioning](https://semver.org/).
> **PT:** Todas as mudanças notáveis do glintfx. Formato baseado em [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); o projeto segue [Versionamento Semântico](https://semver.org/).

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
