# Layer-1-glintfx

## English

**glintfx** is a drop-in C++ library for Linux x86-64 (MPL-2.0 license) that fuses [RmlUi](https://github.com/mikke89/RmlUi) (an HTML/CSS-style UI engine) with a GL3 effects renderer (glow, gradient, backdrop-blur, drop-shadow, mask, image-tint), all declared in `.rcss` (a CSS-like stylesheet), no imperative effect API to learn. This is the project's **active, released product**: current version **v0.9.0**, full history in [`CHANGELOG.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md).

Two ways to consume it:

- `glintfx::App`, standalone, owns its own window and frame loop.
- `glintfx::UiLayer`, embed/guest mode, attaches to a host application's own GL context (its first real-world consumer is a game called GusWorld).

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

glintfx currently depends on three real, mature third-party libraries: RmlUi, GLFW, and FreeType. The project's long-term (multi-year, no-deadline) ambition is to replace pieces of that dependency stack with clean-room reimplementations built on [Layer 0](Layer-0-Core). One piece has already made that journey: glintfx's own OpenGL function-pointer loader, previously the third-party `gl3w`, is now generated in-house from the public Khronos `gl.xml` registry. See ADR-0009 for the exact boundary of what will and will not be internalized.

### Credit / crediting RmlUi

glintfx is built on [RmlUi](https://github.com/mikke89/RmlUi) (MIT License), the HTML/CSS-style UI engine glintfx wraps and drives from `.rcss` -- our thanks to mikke89 and the RmlUi contributors for a mature, well-documented project to build on. When we find and fix a genuine bug in RmlUi itself, we track the fix as an explicit, reviewed source patch applied automatically at `FetchContent` time (`glintfx/patches/`, see `glintfx/patches/README.md`), and aim to report/submit it back upstream. Current example: `rmlui-2cd28864-teardown-ub.patch` fixes two undefined-behavior findings in RmlUi's own document/element teardown order (see [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) section 18 and the [`NOTICE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/NOTICE) file). Every such patch is meant to be temporary: retired once the equivalent fix lands upstream.

---

## Português

O **glintfx** é uma biblioteca C++ drop-in para Linux x86-64 (licença MPL-2.0) que funde o [RmlUi](https://github.com/mikke89/RmlUi) (um motor de UI estilo HTML/CSS) com um renderer de efeitos GL3 (glow, degradê, backdrop-blur, drop-shadow, mask, image-tint), tudo declarado em `.rcss` (uma folha de estilo tipo CSS), sem API imperativa de efeito pra aprender. Este é o **produto ativo e lançado** do projeto: versão atual **v0.9.0**, histórico completo em [`CHANGELOG.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md).

Dois jeitos de consumir:

- `glintfx::App`, standalone, dono da própria janela e do loop de frame.
- `glintfx::UiLayer`, modo embed/guest, anexa ao contexto GL de uma aplicação host (o primeiro consumidor real é um jogo chamado GusWorld).

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

O glintfx hoje depende de três bibliotecas reais e maduras de terceiros: RmlUi, GLFW e FreeType. A ambição de longo prazo do projeto (anos, sem prazo) é substituir pedaços dessa pilha de dependências por reimplementações clean-room construídas sobre a [Camada 0](Layer-0-Core). Uma peça já fez essa jornada: o loader próprio de ponteiros de função OpenGL do glintfx, antes o `gl3w` de terceiro, agora é gerado internamente a partir do registro público Khronos `gl.xml`. Ver ADR-0009 pra fronteira exata do que será e não será internalizado.

### Crédito ao RmlUi

O glintfx é construído sobre o [RmlUi](https://github.com/mikke89/RmlUi) (licença MIT), o motor de UI estilo HTML/CSS que o glintfx embrulha e comanda a partir de `.rcss` -- nosso agradecimento ao mikke89 e aos contribuidores do RmlUi por um projeto maduro e bem documentado sobre o qual construir. Quando encontramos e corrigimos um bug genuíno no próprio RmlUi, rastreamos a correção como um patch de fonte explícito e revisado, aplicado automaticamente em tempo de `FetchContent` (`glintfx/patches/`, ver `glintfx/patches/README.md`), e buscamos reportá-lo/submetê-lo de volta ao upstream. Exemplo atual: o `rmlui-2cd28864-teardown-ub.patch` corrige dois achados de undefined behavior na própria ordem de teardown de documento/elemento do RmlUi (ver [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) seção 18 e o arquivo [`NOTICE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/NOTICE)). Todo patch assim é pensado para ser temporário: aposentado assim que a correção equivalente chegar no upstream.
