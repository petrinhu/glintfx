# Layer-1-glintfx

## English

**glintfx** is a drop-in C++ library for Linux x86-64 (MPL-2.0 license) that fuses [RmlUi](https://github.com/mikke89/RmlUi) (an HTML/CSS-style UI engine) with a GL3 effects renderer (glow, gradient, backdrop-blur, drop-shadow, mask, image-tint, screen-space ripple), all declared in `.rcss` (a CSS-like stylesheet), no imperative effect API to learn. This is the project's **active, released product**: current version **v0.11.0**, full history in [`CHANGELOG.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md).

Two ways to consume it:

- `glintfx::App`, standalone, owns its own window and frame loop.
- `glintfx::UiLayer`, embed/guest mode, attaches to a host application's own GL context (its first real-world consumer is a game called GusWorld).

### What's new since v0.9.0

- **`v0.9.2`:** the generated GL loader gained a Windows codepath (`wglGetProcAddress` + `opengl32.dll`, MinGW-w64 cross-compile-validated) -- a real step for consumers targeting Windows, but **not** a change to the project's officially supported platform, which stays Linux x86-64 (see the main [`README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/README.md#known-limitations)'s "Known limitations").
- **`v0.10.0` (`L1.20-FONTFLIP`):** glintfx's own clean-room font engine -- built on Layer 0's `glx_sfnt`/`glx_raster`/`glx_hint` -- became the **default** rasteriser (`GLINTFX_OWN_FONT_ENGINE=ON` at build time, `FontEngine::Own` at runtime). This is a **soft** flip: FreeType stays fully linked and selectable per-instance, no rebuild required --
  ```cpp
  glintfx::AppConfig cfg;   // or glintfx::UiLayerConfig
  cfg.font_engine = glintfx::FontEngine::FreeType;  // one-line rollback to the pre-v0.10.0 default
  ```
  See [ADR-0011](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0011-soft-font-flip.md) and `docs/embed-integration.md` section 17.
- **`v0.11.0` (`L1.22-WAVE`/`L1.22-CAPTURE`):** a `decorator: ripple(...)` RCSS decorator captures the host's FBO 0 and refracts it screen-space over glintfx's own composed UI -- a shimmer/water-ripple look, driven entirely by RCSS custom properties (`ripple-origin-x`/`-y`/`-phase`/`-strength`/`-width`, all animatable via `@keyframes`), no new C++ API. Embed/`UiLayer` mode only. See [`docs/effects.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/effects.md) for the syntax and [ADR-0012](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0012-backdrop-capture.md) for the design rationale (a documented, opt-in, cost-zero-when-inactive exception to `UiLayer`'s compose-only guarantee).

### Where to go

| Need | Link |
| :--- | :--- |
| First tutorial (assumes C++/CMake) | [`docs/getting-started.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/getting-started.md) |
| Total-beginner explanation | [`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md) |
| Effects syntax (how-to + reference) | [`docs/effects.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/effects.md) |
| Embedding into your own app | [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) |
| Installing a pre-built copy | [`docs/packaging.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/packaging.md) |
| Fixing a build/integration error | [`docs/troubleshooting.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/troubleshooting.md) |
| Design rationale | [`docs/adr/README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/README.md) (see especially ADR-0006, the two-layer split, and ADR-0008, embed mode) |
| Public API (always current, doc-commented) | [`glintfx/include/glintfx/`](https://codeberg.org/petrinhu/glintfx/src/branch/main/glintfx/include/glintfx) |

### The long-term angle

glintfx currently depends on three real, mature third-party libraries: RmlUi, GLFW, and FreeType. The project's long-term (multi-year, no-deadline) ambition is to replace pieces of that dependency stack with clean-room reimplementations built on [Layer 0](Layer-0-Core). Two pieces have already made that journey: glintfx's own OpenGL function-pointer loader, previously the third-party `gl3w`, is now generated in-house from the public Khronos `gl.xml` registry; and, since `v0.10.0`, glintfx's **default** text rasteriser is its own `SOV-SFNT`/`SOV-RAST`/`SOV-HINT` font engine, built on Layer 0 (FreeType stays linked and selectable, see "What's new" above). See ADR-0009 for the exact boundary of what will and will not be internalized.

### Credit / crediting RmlUi

glintfx is built on [RmlUi](https://github.com/mikke89/RmlUi) (MIT License), the HTML/CSS-style UI engine glintfx wraps and drives from `.rcss` -- our thanks to mikke89 and the RmlUi contributors for a mature, well-documented project to build on. When we find and fix a genuine bug in RmlUi itself, we track the fix as an explicit, reviewed source patch applied automatically at `FetchContent` time (`glintfx/patches/`, see `glintfx/patches/README.md`), and aim to report/submit it back upstream. Current example: `rmlui-2cd28864-teardown-ub.patch` fixes two undefined-behavior findings in RmlUi's own document/element teardown order (see [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) section 18 and the [`NOTICE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/NOTICE) file). Every such patch is meant to be temporary: retired once the equivalent fix lands upstream.

---

## Português

O **glintfx** é uma biblioteca C++ drop-in para Linux x86-64 (licença MPL-2.0) que funde o [RmlUi](https://github.com/mikke89/RmlUi) (um motor de UI estilo HTML/CSS) com um renderer de efeitos GL3 (glow, degradê, backdrop-blur, drop-shadow, mask, image-tint, onda screen-space), tudo declarado em `.rcss` (uma folha de estilo tipo CSS), sem API imperativa de efeito pra aprender. Este é o **produto ativo e lançado** do projeto: versão atual **v0.11.0**, histórico completo em [`CHANGELOG.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md).

Dois jeitos de consumir:

- `glintfx::App`, standalone, dono da própria janela e do loop de frame.
- `glintfx::UiLayer`, modo embed/guest, anexa ao contexto GL de uma aplicação host (o primeiro consumidor real é um jogo chamado GusWorld).

### O que há de novo desde a v0.9.0

- **`v0.9.2`:** o loader GL gerado ganhou um codepath Windows (`wglGetProcAddress` + `opengl32.dll`, validado por cross-compile MinGW-w64) -- um passo real pra consumidores mirando Windows, mas **não** uma mudança na plataforma oficialmente suportada do projeto, que continua Linux x86-64 (ver a seção "Limitações conhecidas" do [`README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/README.md#limitações-conhecidas) principal).
- **`v0.10.0` (`L1.20-FONTFLIP`):** o motor de fonte próprio clean-room da glintfx -- construído sobre `glx_sfnt`/`glx_raster`/`glx_hint` da Camada 0 -- virou o rasterizador **default** (`GLINTFX_OWN_FONT_ENGINE=ON` em tempo de build, `FontEngine::Own` em runtime). É um flip **suave**: o FreeType continua plenamente linkado e selecionável por-instância, sem rebuild --
  ```cpp
  glintfx::AppConfig cfg;   // ou glintfx::UiLayerConfig
  cfg.font_engine = glintfx::FontEngine::FreeType;  // rollback de uma linha pro default pré-v0.10.0
  ```
  Ver [ADR-0011](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0011-soft-font-flip.md) e a seção 17 de `docs/embed-integration.md`.
- **`v0.11.0` (`L1.22-WAVE`/`L1.22-CAPTURE`):** um decorator RCSS `decorator: ripple(...)` captura o FBO 0 do host e o refrata em screen-space sobre a própria UI composta da glintfx -- um visual de shimmer/ondulação na água, dirigido inteiramente por propriedades customizadas RCSS (`ripple-origin-x`/`-y`/`-phase`/`-strength`/`-width`, todas animáveis via `@keyframes`), sem API C++ nova. Só em modo embed/`UiLayer`. Ver [`docs/effects.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/effects.md) pra sintaxe e [ADR-0012](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0012-backdrop-capture.md) pra racional de design (uma exceção documentada, opt-in, custo-zero-quando-inativa à garantia compose-only do `UiLayer`).

### Pra onde ir

| Preciso de | Link |
| :--- | :--- |
| Primeiro tutorial (assume C++/CMake) | [`docs/getting-started.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/getting-started.md) |
| Explicação pra iniciante total | [`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md) |
| Sintaxe dos efeitos (how-to + referência) | [`docs/effects.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/effects.md) |
| Embutir no seu próprio app | [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) |
| Instalar uma cópia pré-buildada | [`docs/packaging.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/packaging.md) |
| Resolver um erro de build/integração | [`docs/troubleshooting.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/troubleshooting.md) |
| Racional de design | [`docs/adr/README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/README.md) (ver especialmente ADR-0006, a divisão em duas camadas, e ADR-0008, o modo embed) |
| API pública (sempre atual, doc-commented) | [`glintfx/include/glintfx/`](https://codeberg.org/petrinhu/glintfx/src/branch/main/glintfx/include/glintfx) |

### O ângulo de longo prazo

O glintfx hoje depende de três bibliotecas reais e maduras de terceiros: RmlUi, GLFW e FreeType. A ambição de longo prazo do projeto (anos, sem prazo) é substituir pedaços dessa pilha de dependências por reimplementações clean-room construídas sobre a [Camada 0](Layer-0-Core). Duas peças já fizeram essa jornada: o loader próprio de ponteiros de função OpenGL do glintfx, antes o `gl3w` de terceiro, agora é gerado internamente a partir do registro público Khronos `gl.xml`; e, desde a `v0.10.0`, o rasterizador de texto **default** da glintfx é o próprio motor de fonte `SOV-SFNT`/`SOV-RAST`/`SOV-HINT`, construído sobre a Camada 0 (o FreeType continua linkado e selecionável, ver "O que há de novo" acima). Ver ADR-0009 pra fronteira exata do que será e não será internalizado.

### Crédito ao RmlUi

O glintfx é construído sobre o [RmlUi](https://github.com/mikke89/RmlUi) (licença MIT), o motor de UI estilo HTML/CSS que o glintfx embrulha e comanda a partir de `.rcss` -- nosso agradecimento ao mikke89 e aos contribuidores do RmlUi por um projeto maduro e bem documentado sobre o qual construir. Quando encontramos e corrigimos um bug genuíno no próprio RmlUi, rastreamos a correção como um patch de fonte explícito e revisado, aplicado automaticamente em tempo de `FetchContent` (`glintfx/patches/`, ver `glintfx/patches/README.md`), e buscamos reportá-lo/submetê-lo de volta ao upstream. Exemplo atual: o `rmlui-2cd28864-teardown-ub.patch` corrige dois achados de undefined behavior na própria ordem de teardown de documento/elemento do RmlUi (ver [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) seção 18 e o arquivo [`NOTICE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/NOTICE)). Todo patch assim é pensado para ser temporário: aposentado assim que a correção equivalente chegar no upstream.
