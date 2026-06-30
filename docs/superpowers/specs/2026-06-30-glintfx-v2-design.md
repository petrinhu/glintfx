# Design — glintfx v2: embed mode + component library de UI de jogo

- **Data:** 2026-06-30
- **Status:** Draft (aguarda revisão do líder)
- **Origem:** brainstorm v2 (Capitolino/CPO — lente de produto; ux-ui-designer — lente de design), sobre o veredito de integração GusWorld e o [ADR-0008](../../adr/0008-embed-guest-mode.md)
- **Referências:** [spec v1](2026-06-28-camada1-rmlui-gl3-design.md), [ADR-0006](../../adr/0006-layered-hybrid-architecture.md) (camadas), [ADR-0007](../../adr/0007-license-mpl-2.0.md) (licença MPL-2.0), [ADR-0008](../../adr/0008-embed-guest-mode.md) (embed/guest mode), `TODO.md` (trilha Camada 1)

> Idioma: spec é documento de trabalho interno (pt-br). A API reference pública (item `DOC-WIKI`, via technical-writer) será bilíngue en→pt, como na v1.

## 1. Objetivo e escopo

A v1 entregou o **engine** (fachada `App` + RmlUi + renderer GL3 + efeitos data-driven), provando o north-star "drop-in: linke um alvo e escreva CSS". A v2 sobe um nível: **parar de redesenhar UI a cada projeto.** Entrega duas coisas, nesta ordem:

1. **Embed/guest mode (keystone, ADR-0008):** uma fachada `UiLayer` que anexa o motor de UI+efeitos a um contexto GL que o **host** já possui (jogo com loop/janela próprios), sem criar janela, compondo por cima da cena sem limpar nem dar swap. Sem isto, **nenhum componente serve um jogo** — por isso vem primeiro.
2. **Component library (design system) de UI de jogo:** tokens (2 níveis) + componentes reutilizáveis (Atomic Design) com efeitos GPU, **data-driven** (`.rcss` + templates `.rml`, C++ fino só onde corta boilerplate). 1ª onda priorizada pelas telas **reais do GusWorld**: cockpit/HUD, **caixa de diálogo narrativa**, menu.

**Proposta de valor (north-star da v2):** "v2 = não redesenha UI a cada projeto". 1º consumidor = **GusWorld** (jogo). Depois, distribuição para o portfólio do líder (apps densos + showcase) — tirar apps do visual "anos 90 chapado" sem custo de redesenho por projeto. A v2 é **aditiva**: o engine v1 (`App` standalone) permanece intacto; os componentes entram por um alvo CMake **opt-in** `glintfx::ui`.

Não-objetivos: ver §13.

## 2. Decisões travadas (do brainstorm)

| Tema | Decisão |
|---|---|
| 1º consumidor | **GusWorld** (jogo). v2 = UI de jogo (menus, diálogos, janelas, fontes, efeitos) |
| Keystone | **Embed mode (ADR-0008)** ANTES dos componentes — sequência inegociável |
| Distribuição | **Aditiva**, alvo opt-in **`glintfx::ui`**; engine v1 intacto |
| Abordagem | **Tokens-first**, **data-driven** (tokens.rcss + glintfx.rcss + templates RML); C++ fino só onde corta boilerplate |
| Tema | **1 tema default** no MVP + variantes via `@media (theme:…)`; override do consumidor por `overrides.rcss` |
| Versionamento | **Mono-versão** (a v2 acompanha a versão da lib; sem matriz de versões) |
| Navegação | **Gamepad é requisito** (RmlUi nav + eventos injetados; GusWorld usa SDL3 gamepad AAA) |
| Efeitos | **Modificadores ortogonais** (`.glow/.glass/.gradient/.masked`) → **N+M regras, não N×M** |
| a11y | Limite honesto: RmlUi **não tem screen reader/ARIA**. a11y = foco + teclado/gamepad + contraste AA + high-contrast + reduced-motion + cor nunca sozinha |
| Escopo | **Anti-OE forte:** só entra componente com **caso de uso real** (telas do GusWorld). 1ª onda enxuta |
| Licença | **MPL-2.0** (projeto inteiro, ADR-0007); SPDX em todo arquivo |

## 3. Arquitetura — dois modos de consumo

A v2 mantém o engine v1 e adiciona um segundo modo de consumo, mais o pacote de componentes. Nada do v1 é removido ou reescrito.

```
                 ┌──────────────────────── glintfx::ui (NOVO, opt-in) ───────────────────────┐
                 │  tokens.rcss (primitivo→semântico) · glintfx.rcss (componentes+modifiers)  │
                 │  templates/*.rml (button, field, modal, dialog, …) · helpers C++ finos      │
                 └───────────────┬───────────────────────────────────────────┬───────────────┘
                                 │ data-driven (RML/RCSS), sem GL no consumidor │
   ┌─────────── MODO A: standalone (v1) ───────────┐   ┌──── MODO B: embed/guest (ADR-0008) ────┐
   │  glintfx::App  — dono da janela (GLFW),        │   │  glintfx::UiLayer — anexa ao contexto    │
   │  loop, contexto GL, eventos. App só-de-UI.     │   │  GL do HOST; compose-only; eventos       │
   │  [INTACTO da v1]                                │   │  injetados; save/restore de estado GL.   │
   └───────────────────┬────────────────────────────┘   └──────────────────┬─────────────────────┘
                       │                                                     │
                 ┌─────▼─────────────── núcleo compartilhado (v1) ──────────▼─────┐
                 │  M0 Bootstrap (Rml::Initialise/Context) · M2 Render (GL3) ·     │
                 │  M1 Font (FreeType)  — uma só implementação de motor             │
                 └─────────────────────────────────────────────────────────────────┘
```

- **Modo A — `App` standalone (v1, intacto):** o dev que só quer UI (apps de portfólio, showcase) continua usando `App` (dono de janela/loop). Zero mudança.
- **Modo B — `UiLayer` embed (novo, ADR-0008):** o jogo (GusWorld) é dono da janela/contexto/loop (SDL3) e da cena (`Render2dGl3`); a `UiLayer` compõe a UI por cima. É o que destrava a v2 para jogos.
- **`glintfx::ui` (componentes):** tokens + folhas + templates RML, consumíveis **identicamente** pelos dois modos. Um componente NUNCA assume qual modo o dirige (requisito de produto: componente serve tanto `App` quanto `UiLayer`).
- **Núcleo único:** o `App` standalone pode ser reexpresso internamente como um host fino (janela GLFW) que dirige uma `UiLayer`, mantendo **um só caminho de engine** (M0/M1/M2). Isso evita duplicar o ciclo de frame.

## 4. Embed/guest mode (detalhe — keystone)

Formalizado no [ADR-0008](../../adr/0008-embed-guest-mode.md). Resumo operacional:

**(a) Anexar, não criar.** `UiLayer` é construída contra um contexto GL **já corrente** do host (criado por SDL3/GLFW/X11 — indiferente). Carrega ponteiros GL via gl3w (idempotente) e inicializa RmlUi + `RenderInterface_GL3`. **Nunca abre janela.**

**(b) Frame compose-only.** `render()` emite **só** a geometria da UI: **sem `glClear`, sem swap**. O host limpa, desenha a cena, chama `UiLayer::render()` e faz o **swap único**. Generaliza o `BeginFrame`/`EndFrame` do RmlUi para compor sobre o framebuffer do host (premultiplied-alpha) sem apagá-lo.

**(c) Eventos injetados.** O host traduz seus eventos nativos e empurra na `UiLayer` (mouse/teclado/texto/resize/clock). Um host SDL3 usa seu próprio adapter (GusWorld já tem `RmlUi_Platform_SDL`). **Navegação por gamepad** entra aqui: o host mapeia o gamepad para os comandos de navegação do RmlUi (foco direcional, ativar, voltar) e injeta como eventos de teclado/navegação. O engine não assume GLFW.

**(d) Contrato de estado GL.** Com dois donos no mesmo contexto, `render()` **salva e restaura** o estado GL que toca (viewport, blend, scissor, program/VAO/texturas vinculados, depth/stencil) — e documenta as invariantes que confia ao host respeitar.

Esboço da API (ilustrativo; nomes/assinaturas finais na tarefa de build — ver ADR-0008):

```cpp
namespace glintfx {
class UiLayer {                       // anexa a um contexto GL host-current; não possui janela
public:
  struct Config { int logical_width=1280, logical_height=720; bool load_gl=true; };
  explicit UiLayer(Config cfg = {});
  bool ok() const noexcept;
  void load(const char* rml_path);
  void set_pixel_size(int w, int h);            // resize do alvo real
  // eventos injetados pelo host (SDL3, etc.)
  void push_mouse_move(float x, float y);
  void push_mouse_button(int button, bool down);
  void push_key(int rml_key, bool down);        // inclui navegação por gamepad mapeada
  void push_text(const char* utf8);
  void update();                                // layout/animações/data-model
  void render();                                // COMPOSE-ONLY: sem clear, sem swap; salva/restaura GL
private:
  struct Impl; std::unique_ptr<Impl> impl_;
};
}
```

## 5. Tokens (design tokens — 2 níveis)

Tokens vivem em `tokens.rcss` como propriedades customizadas, em **dois níveis** (lente de design):

- **Nível 1 — primitivos:** valores crus, sem semântica. Ex.: paleta (`--c-ink-900: #0b0e14ff`), escala de cinza, cores de marca.
- **Nível 2 — semânticos:** referenciam primitivos e carregam intenção. Ex.: `--color-bg`, `--color-surface`, `--color-text`, `--color-accent`, `--color-focus`, `--color-danger`. **O consumidor e os componentes usam só os semânticos** (trocar tema = remapear semântico→primitivo, sem tocar componente).

Categorias de token:

| Categoria | Conteúdo | Nota |
|---|---|---|
| Cor | primitivos + semânticos, hex **`#rrggbbaa`** (alpha explícito) | redefinidos por `@media (theme: dark)` / `high-contrast` |
| Espaçamento | escala **4 / 8 / 16 / 24 / 40** | `--space-1..5` |
| Raio | `--radius-sm/md/lg/pill` | usados por button/card/modal |
| Tipografia | família, tamanhos, line-height, peso (FreeType) | 1 família default no MVP |
| Elevação | `box-shadow` **com a cor PRIMEIRO**, como **string** (`--elev-1: …`) | sombra é string, não decomposta |
| Motion | `--motion-fast/base`, easing; `--motion: 0s` em reduced-motion | desliga animação por token |
| **Efeito (paramétrico)** | `--glow-color / --glow-blur / --glow-spread`, `--blur-radius`, `--grad-a / --grad-b / --grad-angle` | efeito é **paramétrico** via token, não hardcode |

Os tokens de efeito são o que torna os modificadores (§7) **temáveis**: o componente declara `box-shadow: 0 0 var(--glow-blur) var(--glow-spread) var(--glow-color)`, e o tema/consumidor ajusta os valores.

## 6. Componentes (taxonomia atômica + 1ª onda)

Taxonomia Atomic Design mapeada a RmlUi. **Preferir elementos nativos** (`<button>`, `<input>`) — eles trazem foco e teclado de graça (base da a11y, §8).

| Nível | Componentes | Mapeamento RmlUi |
|---|---|---|
| **Atoms** | button, input, label, icon, badge | `<button>`, `<input type=text/range/checkbox>`, `<span>`, sprite/glyph, `<span class=badge>` |
| **Molecules** | field (label+input+erro), card | composição de atoms |
| **Organisms** | navbar, modal, panel, **dialog box** | containers + template RML |

**1ª onda (enxuta, priorizada pelas telas reais do GusWorld) — cada um com pelo menos um efeito:**

1. **button** (atom) — 3 estados (normal/hover/active) + foco visível; variante com `.glow`. Base do menu e do cockpit.
2. **panel / window** (organism) — moldura com `.gradient`/`.glass`; base do cockpit/HUD.
3. **dialog box** (organism) — caixa de diálogo **narrativa** do GusWorld: retrato + nome + texto + indicador de avançar; `.glass`/`.gradient`. **Item-chave do 1º consumidor.**
4. **menu / nav list** (organism) — lista navegável por **teclado e gamepad** (foco direcional); base dos menus.
5. **label + badge** (atoms) — texto e status (HP/AP/contadores) do HUD.
6. **field** (molecule) — só se uma tela real exigir entrada de texto (ex.: rename/settings); senão fica para onda 2 (anti-OE).

Cada componente = um trecho de `glintfx.rcss` (estilo) + opcionalmente um template `.rml` reutilizável + helper C++ fino **só** se cortar boilerplate repetitivo (ex.: abrir/fechar modal). **Nenhum C++ novo por componente** como regra; C++ é exceção justificada.

Anti-OE: **nada de** componentes "porque um design system costuma ter" (date picker, accordion, carousel, tooltip…). Entram quando uma tela real pedir.

## 7. Efeitos como modificadores ortogonais

Os efeitos GPU (do `RenderInterface_GL3`, já existentes) são expostos como **classes modificadoras** que o consumidor combina no markup, **ortogonais** aos componentes:

```html
<button class="glow">OK</button>
<div class="panel glass gradient">…</div>
```

Isso dá **N + M regras** (N componentes + M efeitos), **não N × M** (não pré-multiplicamos "botão-com-glow", "botão-com-glass", …): a combinação é feita pelo consumidor no markup. É a decisão central que mantém a biblioteca enxuta e composável.

| Modificador | Efeito | Status |
|---|---|---|
| `.glow` | drop-shadow/box-shadow colorido (paramétrico via `--glow-*`) | **1ª classe** |
| `.glass` | `backdrop-filter: blur(--blur-radius)` + fundo translúcido | **1ª classe** |
| `.gradient` | `linear/radial-gradient(--grad-a, --grad-b, --grad-angle)` | **1ª classe** |
| `.masked` | `mask` (recorte por imagem/forma) | **só GPU real** — ver aviso |

**Aviso `.masked`:** o caminho de `mask` exige GPU de verdade; **crasha no `llvmpipe`** (rasterizador de software, usado em CI headless sem GPU). Portanto `.masked` é documentado como "requer GPU", **não** entra no smoke de CI por software, e os testes que o exercitam rodam só com GPU disponível (§11).

## 8. Theming, dark mode, gamepad e a11y

**Theming.** Variantes redefinem os **tokens semânticos** no `body` via media query, sem tocar componente:

```css
@media (theme: dark)          { body { --color-bg: …; --color-text: …; } }
@media (theme: high-contrast) { body { --color-bg:#000000ff; --color-text:#ffffffff; --color-focus:#ffee00ff; } }
@media (theme: reduced-motion){ body { --motion: 0s; } }
```

A `App`/`UiLayer` expõe API para ativar um tema (ex.: setar a media feature ativa). O **consumidor sobrescreve** carregando um `overrides.rcss` depois do `glintfx.rcss` (cascata).

**Tema no MVP:** **1 tema default** (escuro, casa com GusWorld) + as variantes acima. Sem editor de tema, sem múltiplos temas prontos (anti-OE).

**Gamepad (requisito).** A navegação direcional + ativar/voltar usa a navegação de foco do RmlUi, alimentada por **eventos injetados** (§4c). O host (GusWorld/SDL3) mapeia d-pad/sticks/botões → comandos de navegação. Componentes da onda 1 que navegam (menu, dialog, modal) são desenhados foco-first para funcionar **sem mouse**.

**a11y — limite honesto.** RmlUi **não tem screen reader nem ARIA**; prometer "acessível" seria desonesto. O que entregamos e auditamos:

- **Foco visível** sempre (estilo de `:focus` com `--color-focus`, nunca removido).
- **Navegação por teclado e gamepad** completa (atoms nativos + foco direcional).
- **Contraste AA:** texto ≥ **4.5:1**, UI/foco ≥ **3:1**. **Auditar a paleta do POC** contra esses limites.
- **Variante high-contrast** (token override).
- **Reduced-motion** (`--motion: 0s` desliga animações).
- **Cor nunca sozinha** como portadora de informação (status sempre tem ícone/texto além da cor).

## 9. Distribuição e versionamento

- **Alvo opt-in `glintfx::ui`.** Quem só quer o engine linka `glintfx::glintfx` (v1, intacto). Quem quer os componentes adiciona `glintfx::ui`, que traz `tokens.rcss` + `glintfx.rcss` + `templates/*.rml` (+ helpers finos). **Aditivo:** não muda o consumo de quem está na v1.
- **Assets de UI** (rcss/rml/fontes default) instalados/empacotados junto, resolvíveis em runtime (mesma lógica de resolução de asset que a v1 já usa).
- **Mono-versão.** A v2 acompanha a versão da lib (sem matriz de versões dos componentes). SemVer no repositório único.
- **Consumo:** `FetchContent`/`find_package` (já suportados na v1) + `target_link_libraries(app PRIVATE glintfx::ui)`.

## 10. Integração GusWorld (resumo do ADR-010, faseado)

Detalhe no rascunho do **ADR-010 do GusWorld** (repo do GusWorld). Resumo:

- GusWorld adota o glintfx via **embed mode** (`UiLayer`), modelo B do veredito, e **aposenta** `RmlUi_Renderer_GL3` + `gl3_loader` + o miolo do `rmlui_hud` vendorizados à mão. Mantém **SDL3** (ADR-008 deles: janela/input/gamepad/áudio), a **arena `Render2dGl3`** e a ordem de composição arena→HUD→swap.
- O `RmlUi_Platform_SDL` deles **permanece** como adapter de eventos injetados (§4c).
- Versão: GusEngine sobe RmlUi 6.2→**6.3** (ou a v2 confirma compat 6.2).
- `core/`+`domain/` POCO (~1013 testes) **intactos**; o GATE das 4 camadas continua.
- Sequência: **embed mode pronto → adoção no GusWorld**.

## 11. Testes (qa-engineer)

- **Embed mode — smoke headless:** inicializa `UiLayer` contra um contexto GL offscreen, injeta eventos roteirizados, faz `update()`+`render()` compose-only, captura o **FBO offscreen** e sai sem erro/leak. Prova attach + compose-only + restauração de estado GL (verificar que o estado GL volta ao valor pré-`render()`).
- **Componentes — golden-image:** uma cena por componente da onda 1 (button/panel/dialog/menu/label) renderizada e comparada a referência com tolerância de pixel. Cobre estados (normal/hover/active/focus) onde aplicável.
- **Theming:** golden por variante crítica (default + high-contrast) de pelo menos 1 componente, garantindo que o override de token reflete.
- **a11y mecânica:** teste de **contraste** automatizado sobre os tokens semânticos (calcula razão e falha se < AA); teste de **navegação por foco** (sequência de `push_key` move o foco como esperado, inclui o caminho do gamepad mapeado).
- **`.masked`:** **fora** do smoke de CI por software (llvmpipe crasha); roda só em runner com GPU; documentado.

## 12. Faseamento

1. **F1 — Embed mode (`UiLayer`)** [keystone, ADR-0008]: attach a contexto host, compose-only, eventos injetados, save/restore de estado GL, smoke headless via FBO. **Bloqueia tudo abaixo.**
2. **F2 — Tokens + componentes onda 1**: `tokens.rcss` (2 níveis) + `glintfx.rcss` (componentes + modificadores `.glow/.glass/.gradient`) + templates RML; button/panel/dialog/menu/label; theming default + high-contrast + reduced-motion; golden + contraste. `.masked` documentado (GPU-only).
3. **F3 — Adoção no GusWorld**: GusWorld migra para `UiLayer` + `glintfx::ui` (cockpit/HUD, dialog, menu); aposenta o backend vendorizado; valida paridade visual + pacing/motor verdes (ADR-010 deles).
4. **F4 — Portfólio**: empacotar `glintfx::ui` para apps densos do líder + showcase v2; novos componentes/temas **só** com caso de uso real.

## 13. Não-objetivos (YAGNI / anti-OE)

- **Não** Windows/macOS/SDL/X11 como backend do `App` v1 (embed mode já cobre hosts arbitrários — o host traz seu backend).
- **Não** design system "completo" (sem date picker, accordion, carousel, tooltip, table… sem caso de uso real).
- **Não** múltiplos temas prontos, editor de tema, ou hot-reload de tema no MVP (1 default + variantes por media query).
- **Não** API imperativa de efeito (segue data-driven via CSS, como a v1).
- **Não** screen reader / ARIA (limite honesto do RmlUi; ver §8).
- **Não** matriz de versões dos componentes (mono-versão).
- **Não** internalização clean-room de RmlUi/GL nesta v2 (segue como trajetória de longo prazo, ADR-0006).
- **Não** C++ por componente como padrão (data-driven; C++ fino só como exceção justificada).

## 14. Riscos

- **R1 — Sequência:** os componentes não servem jogos sem o embed mode. Mitigado por faseamento (F1 antes de F2/F3).
- **R2 — Estado GL compartilhado** (dois donos, `UiLayer` + renderer do host): bug sutil. Mitigado pelo contrato save/restore (§4d) + teste que verifica restauração + doc de invariantes.
- **R3 — `.masked` em software (llvmpipe):** crash. Mitigado por marcar GPU-only, excluir do CI por software, documentar.
- **R4 — Skew de versão do RmlUi** (glintfx 6.3 × GusWorld 6.2): mitigado por alinhar a versão na F3 (ou confirmar compat 6.2).
- **R5 — a11y mal-comunicada:** prometer mais do que o RmlUi entrega. Mitigado pelo "limite honesto" explícito (§8) na doc pública.
- **R6 — Scope creep do design system:** pressão por componentes "padrão". Mitigado pela regra "só com caso de uso real" (§2, §13) e pela 1ª onda amarrada às telas do GusWorld.
- **R7 — Contraste do POC abaixo de AA:** a paleta atual pode falhar. Mitigado pela auditoria de contraste automatizada (§11) e ajuste de tokens.
