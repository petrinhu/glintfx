# Design — Camada 1: lib C++23 RmlUi + GL3 (v1)

- **Data:** 2026-06-28
- **Status:** Draft (aguarda revisão do líder)
- **Origem:** brainstorm bigtech (Capitolino/CPO, Caetano/CTO, Cósimo/CoS, Cláudio/CLO)
- **Referências:** [ADR-0006](../../adr/0006-layered-hybrid-architecture.md) (camadas), [ADR-0007](../../adr/0007-license-mpl-2.0.md) (licença), `TODO.md` (trilha Camada 1)

> Idioma: spec é documento de trabalho interno (pt-br). A API reference pública (item `DOC-WIKI`, via technical-writer) será bilíngue en→pt.

## 1. Objetivo e escopo

Biblioteca **C++23** (compat C++17→23), **Linux-only**, para **adoção por devs externos**, que unifica numa fachada coesa: **UI** (RmlUi, layout HTML/CSS) + **renderer GL3** (efeitos GPU). A v1 é um **showcase de efeitos**.

Não-objetivos da v1: Windows/macOS; SDL/X11; internalização clean-room; API imperativa de efeitos; editor/hot-reload; empacotamento/distribuição; otimização do renderer.

## 2. Decisões travadas (do brainstorm)

| Tema | Decisão |
|---|---|
| Cliente | Devs externos |
| Plataforma | Linux only (restaura coerência com a Camada 0 Linux) |
| Janela/contexto | **GLFW** primeiro (cobre Linux); interface pronta p/ SDL depois |
| Efeitos | Todos (glow, degradê, blur, mask, backdrop-filter) — **data-driven via CSS**, vêm do `RenderInterface_GL3` real |
| Fonte | **Plugável** (`FontEngineInterface`); FreeType como impl. padrão |
| Build | CMake (`add_subdirectory` RmlUi+gl3w); Makefile da Camada 0 invocado via `add_custom_target` |
| Licença | **MPL-2.0** (projeto inteiro, ADR-0007) |
| Abordagem | Fachada fina + costuras (interfaces com 1 impl, prontas p/ troca) |

## 3. Arquitetura (4 módulos)

```
[ app do dev ]
      │  API pública (1 header C++23, RAII)
┌─────▼─────────────────────────────────────────────┐
│ M3 Facade — App / Window / UiSurface (RAII)        │
├──────────────┬───────────────┬─────────────────────┤
│ M1 Platform  │ M2 Render     │ M0 Bootstrap        │
│ IWindowBackend│ wrap do      │ Rml::Initialise,    │
│ (GLFW)        │ RenderIface  │ Context, load       │
│ IFontEngine   │ _GL3 (as-is) │ .rml/.rcss          │
│ (FreeType)    │              │                     │
└──────────────┴───────────────┴─────────────────────┘
   ↑ nada de GL/janela/RmlUi vaza para o consumidor
```

- **M0 Bootstrap** — ciclo de vida do RmlUi: `Rml::Initialise`/`Shutdown`, criação de `Context`, carga de documento `.rml` + folhas `.rcss`. Pluga nossos `SystemInterface`, `RenderInterface`, `FontEngineInterface`.
- **M1 Platform** — abstração fina de SO/janela:
  - `IWindowBackend` { `create(title,w,h)`, `gl_proc_loader()`, `swap()`, `poll_events()`, `size()`, `should_close()` } — impl. **GLFW** (reusa `SystemInterface_GLFW` do RmlUi para clock/clipboard/cursor).
  - `IFontEngine` = `Rml::FontEngineInterface` — impl. padrão **FreeType** (RmlUi buildado com `RMLUI_FONT_ENGINE=none` + nossa engine plugada).
- **M2 Render** — possui e pluga o `RenderInterface_GL3` **praticamente intacto** (não reescrever). Expõe `begin_frame()`, `end_frame()`, `set_viewport(w,h)`.
- **M3 Facade** — a cara pública: objeto `Application`/`UiSurface` RAII que esconde a tríade e roda o loop. Único header que o consumidor inclui.

**Fronteira de ouro:** a fachada só conversa com o RmlUi via as três interfaces. Nenhum tipo de GL/GLFW/RmlUi aparece na API pública → permite internalização futura sem quebrar a API.

## 4. Fluxo de execução

```
App app{cfg};                       // RAII: GLFW init, janela, contexto GL3 (gl3w carrega),
                                    // Rml::Initialise + plug de System/Render/Font
auto doc = app.load("ui/showcase.rml");
doc.show();
while (app.running()) {             // loop:
    app.poll_events();             //   GLFW events → Rml input
    app.update();                  //   context.Update()
    app.render();                  //   begin_frame → context.Render() → end_frame → swap
}
// destrutor RAII: Shutdown ordenado (Rml → GL → janela)
```

## 5. Efeitos (data-driven)

Todos os efeitos já existem no `RenderInterface_GL3` (2171 linhas de GLSL) e são acionados por **CSS** no `.rcss`:
`filter: blur(4px)`, `drop-shadow(...)`, `box-shadow`, `background: linear/radial/conic-gradient(...)`, `mask`, `backdrop-filter`. A lib **não** expõe API imperativa de efeito; no máximo helpers que injetam/alteram CSS. Custo marginal de "todos os efeitos" = escrever os `.rcss` de demo do showcase.

## 6. Engine de fonte plugável

`IFontEngine` espelha `Rml::FontEngineInterface`. Impl. v1 = **FreeType** (lê qualquer TTF/OTF/WOFF). Trocável por outra engine sem mexer no resto — incluindo uma engine **própria clean-room** no futuro (candidato de internalização). Atende o requisito "usar qualquer fonte, não só FreeType".

## 7. Build (CMake)

- RmlUi e gl3w via `add_subdirectory` (versão pinada do `examples/`), com `RMLUI_SAMPLES OFF`, `BUILD_SHARED_LIBS OFF`, `RMLUI_FONT_ENGINE none` (plugamos a nossa).
- Deps de sistema (Fedora 44): `glfw-devel`, `freetype-devel`, `mesa-libGL-devel`.
- A Camada 0 (Makefile artesanal) **não** se linka à Camada 1 (C++/libGL/libc vs freestanding). O CMake só a invoca via `add_custom_target` (fronteira de **processo**) para haver um "build tudo".

## 8. Fronteira de soberania e internalização

- **Candidatos clean-room (valem, meses):** gl3w (loader), cola de janela/loop, parsers `.rml`/`.rcss` simples, math, engine de fonte própria.
- **Caro mas possível (anos):** motor de layout HTML/CSS; renderer de efeitos GLSL próprio.
- **Fora do alvo (irredutível):** **libGL / driver de GPU / servidor gráfico** — território de fabricante de driver. **A soberania gráfica para no syscall + driver de GPU.** (Espelha a regra "única dependência externa irredutível" adotada pelo líder.)

## 9. Estrutura no repositório

```
src/            Camada 0 (C/ASM, inalterada)
layer1/         Camada 1 (C++23)
  include/      headers públicos da lib (a fachada)
  src/          M0–M3
  demos/        showcase.rml/.rcss + main do demo
  CMakeLists.txt
examples/       upstreams clonados (gitignored): RmlUi, gl3w
```
Nome final da lib: a definir (detalhe menor, resolver no plano).

## 10. Testes (qa-engineer)

- **Smoke do MVP:** builda, abre janela, renderiza o documento, fecha sem erro/leak (exit code).
- **Golden-image:** screenshot do showcase comparado a uma imagem de referência com tolerância de pixel, para não regredir os efeitos.
- Sem outros testes visuais de UI na v1. (TST-* da Camada 1 entram no `TODO.md` quando o plano for gerado.)

## 11. Licença e atribuição

Projeto inteiro sob **MPL-2.0** (ADR-0007). Header SPDX `SPDX-License-Identifier: MPL-2.0` em todo arquivo (`STD-SPDX`). Atribuições das deps linkadas (FreeType FTL, RmlUi MIT, libGL MIT/SGI; gl3w domínio público) no `NOTICE`. Relicenciar antes de aceitar contribuição externa (autor único hoje).

## 12. Definition of Done da v1

1. `cmake` builda a lib + o demo linkando RmlUi/gl3w/GLFW/FreeType reais.
2. Janela GLFW abre com contexto GL3.
3. Documento `showcase.rml`/`.rcss` renderiza com **o leque de efeitos** visível (glow, degradê, blur, mask, backdrop-filter) e **texto** (FreeType).
4. Loop trata fechar janela, resize (`set_viewport`) e input de mouse.
5. Fachada C++23 RAII num header; nada de GL/GLFW/RmlUi vaza na API pública.
6. Smoke + golden-image passam.
7. SPDX MPL-2.0 em todos os arquivos novos; `NOTICE` presente.

## 13. Riscos

- **R1** Dep sub-contada já mapeada (FreeType) — coberta. Confirmar pacotes no ambiente.
- **R2** "Ilusão de soberania" no gráfico — mitigada pela fronteira (§8).
- **R3** Backend sample-grade — mitigado reescrevendo M1/M3 (não copiar cego o ciclo do contexto GL).
- **R4** ABI C++ na fronteira (se virar lib binária distribuível) — fora do escopo solo/MVP; registrar.
- **R5** Acoplamento à versão do RmlUi — mitigado por `add_subdirectory` com versão pinada.
- **R6 [legal]** Formalizar MPL-2.0/contribuições pode pedir advogado humano antes de abrir PRs externos.
