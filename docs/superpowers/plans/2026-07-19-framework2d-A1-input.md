# Framework 2D -- A1: input + frame loop no `App` standalone / A1: own input + frame loop for standalone `App`

> **EN (summary):** Execution plan for slice A1 of the framework mode ([ADR-0015](../../adr/0015-framework-2d-atomized-architecture.md)):
> give the standalone `App` a physical input route (GLFW callbacks translated into the
> EXISTING neutral `UiEvent` path the embed mode already uses; zero duplication of the
> UiEvent-to-RmlUi translation) plus a public `process_event()` for synthetic injection and a
> game frame callback (`set_frame_callback(dt)`, GL context current, scene drawn UNDER the UI).
> Verified finding: today `App::poll_events()` only calls `glfwPollEvents()`; there is not a
> single `glfwSet*Callback` in `glintfx/src/` and no `process_event` on `App`, so physical
> mouse/keyboard NEVER reaches the UI in standalone mode. Delivery discipline: 1 sonnet
> implementer + 1 sonnet adversarial reviewer that EXECUTES (surfaces overlap too much on
> `app.*`/`engine.*` for two parallel implementers).
>
> **PT:** Plano de execução da fatia A1. Autor: Caetano (CTO), 2026-07-19. Branch:
> `feat/framework2d`. Status: aprovado pelo líder como primeira fatia de CAPACIDADE do
> framework mode.
>
> **Ordem canônica: A0 vem PRIMEIRO.** A sequência do framework mode (doc de visão, Opção A)
> começa pela fase **A0 "esqueleto modular"** (refactor de build instalando a família
> `GLINTFX_MODULE_*` + alvos + presets sobre o embrião `GLINTFX_BACKEND_GLFW`, SEM API nova)
> e só então o A1. Este plano é robusto aos dois cenários: **(a) A0 já feito** -- os arquivos
> da seção 3 aterrissam sob os alvos/flags novos do módulo correspondente (window/ui/app),
> mesmo desenho; **(b) A1 adiantado antes do A0** (fast-track do líder) -- o código nasce sob
> o gate existente `GLINTFX_BACKEND_GLFW`, posicionado conforme a regra (f) do
> [ADR-0015](../../adr/0015-framework-2d-atomized-architecture.md) pra que o split futuro de
> flag seja movimento de arquivo de build, não refactor de source. O orquestrador confirma
> com o líder qual cenário vale antes de disparar o implementer.

---

## 1. Estado real do código (verificado nesta análise, não assumido)

Fatos conferidos por leitura e grep em 2026-07-19 (branch `feat/framework2d`):

1. **O `App` não tem NENHUMA rota de input físico.** `App::poll_events()`
   (`glintfx/src/app.cpp:167-172`) chama só `WindowGlfw::poll()`, que chama só
   `glfwPollEvents()` (`glintfx/src/window_glfw.cpp:41`). Grep em `glintfx/src/` +
   `glintfx/include/`: **zero** ocorrências de `glfwSetKeyCallback`, `glfwSetMouseButtonCallback`,
   `glfwSetCursorPosCallback`, `glfwSetScrollCallback`, `glfwSetCharCallback` (o único hit de
   `glfwSet*` é o próprio poll). Os eventos são bombeados pela fila do GLFW e **descartados**:
   mouse e teclado físicos nunca chegam ao RmlUi no modo standalone.
2. **`App` não tem `process_event`.** A lacuna já estava anotada no CLAUDE.md ("App não
   encaminha wheel; não tem process_event nenhum, lacuna pré-existente"). Confirmada no header
   público (`glintfx/include/glintfx/app.hpp`): nenhum método de evento.
3. **A rota neutra existe e está madura, mas só no embed.** `UiLayer::process_event(const UiEvent&)`
   (`glintfx/src/ui_layer.cpp:479-607`) traduz `UiEvent` -> chamadas `Rml::Context::Process*`,
   com guards de não-finito (AUD-TEC-2), offset de sub-viewport (F3) e semântica documentada
   por tipo. Os helpers de tradução `to_rml_key` (`ui_layer.cpp:51`) e `to_rml_mods`
   (`ui_layer.cpp:75`) são `static` DENTRO de `ui_layer.cpp` -- hoje inacessíveis ao `App`.
4. **O `Engine` é o núcleo compartilhado que `App` e `UiLayer` já possuem** (`app.cpp` tem
   `Engine engine` no Impl; `ui_layer.cpp` idem) e já expõe `context()` (`engine.hpp:103`).
   É o lugar natural da tradução compartilhada.
5. **`WindowGlfw` é a única TU do backend de janela** (`window_glfw.cpp`, 48 linhas), expõe
   `handle()` e é compilada só com `GLINTFX_BACKEND_GLFW=ON` (`glintfx/CMakeLists.txt:383`).
6. **`App::run()`** (`app.cpp:431-437`) é `poll + update + render` bloqueante; `render()` chama
   `engine.render_standalone(w, h)` que faz clear + UI + alpha-fix no FBO 0. Não existe gancho
   pro jogo desenhar a própria cena.

Consequência: o A1 é, na prática, **terminar o `App`** reusando a rota `UiEvent` que o embed
já validou em produção -- não é construir um sistema de input do zero.

## 2. Desenho (coerente com o ADR-0015)

Três peças, cada uma no módulo a que pertence (regra (f) do ADR-0015: código pré-A0 aterrissa
posicionado pro split futuro de flag ser movimento de build, não refactor):

### 2.1 Tradução `UiEvent` -> RmlUi sobe pro `Engine` (módulo ui; reuso, não duplicação)

- Novo método interno `Engine::process_event(const UiEvent& ev, int offset_x = 0, int offset_y = 0)`
  em `engine.hpp`/`engine.cpp`: recebe o switch HOJE em `UiLayer::process_event` para os casos
  `None`/`MouseMove`/`MouseButton`/`Key`/`Text`/`MouseWheel`, incluindo os guards de
  não-finito e a subtração de offset no `MouseMove` (parametrizada: o `UiLayer` passa
  `impl_->x, impl_->y`; o `App` passa `0, 0` -- mesma racional do "App é dono da janela
  inteira" já usada em `get_element_box`).
- `to_rml_key`/`to_rml_mods` movem de `ui_layer.cpp` para `engine.cpp` (continuam `static`,
  nunca em header público).
- O caso **`Resize` FICA em `UiLayer::process_event`** (mexe em estado do próprio facade:
  `impl_->w/h`, recomputação de `gl_offset_y` do letterbox). `UiLayer::process_event` vira:
  trata `Resize` local; delega o resto a `engine.process_event(ev, impl_->x, impl_->y)`.
  Refactor de comportamento IDÊNTICO pro embed -- a suíte embed inteira é o guarda.

### 2.2 Captura física no `WindowGlfw` (módulo window)

- `WindowGlfw::set_event_sink(std::function<void(const UiEvent&)>)`: registra em `create()`
  os 5 callbacks GLFW (`glfwSetKeyCallback`, `glfwSetMouseButtonCallback`,
  `glfwSetCursorPosCallback`, `glfwSetScrollCallback`, `glfwSetCharCallback`) com
  `glfwSetWindowUserPointer(win_, this)` + trampolins `static` de UMA linha que chamam
  funções puras de tradução e repassam o `UiEvent` ao sink. Sink nulo = descarte silencioso
  (comportamento atual preservado).
- **Funções puras de tradução** num header interno novo `glintfx/src/glfw_event_translate.hpp`
  (+ `.cpp` se precisar), assinaturas sobre `int`/`double` crus (testáveis sem janela):
  - **Teclado:** `GLFW_KEY_UP/DOWN/LEFT/RIGHT -> Key::Up/Down/Left/Right`;
    `GLFW_KEY_ENTER` E `GLFW_KEY_KP_ENTER -> Key::Enter`; `ESCAPE/TAB/SPACE/BACKSPACE/DELETE/`
    `HOME/END/PAGE_UP/PAGE_DOWN` -> os `Key` correspondentes. Tecla fora do subconjunto =
    **descartada** (não emitir `Key::None`, que viraria `KI_UNKNOWN`; manter a semântica
    inerte da fronteira). `GLFW_PRESS -> pressed=true`, `GLFW_RELEASE -> false`,
    `GLFW_REPEAT -> pressed=true` (repeat gera KeyDown repetido, que é o que o RmlUi espera
    pra navegação segurada).
  - **Modificadores:** mapa EXPLÍCITO `GLFW_MOD_SHIFT -> Mod_Shift`, `GLFW_MOD_CONTROL ->
    Mod_Ctrl`, `GLFW_MOD_ALT -> Mod_Alt`. Os valores numéricos coincidem HOJE, mas proibido
    depender da coincidência (mesma disciplina do `to_rml_mods`).
  - **Botões:** `GLFW_MOUSE_BUTTON_LEFT/RIGHT/MIDDLE` (0/1/2) -> `button` 0/1/2 do `UiEvent`
    (convenção já bate); botões > 2 descartados.
  - **Cursor:** `glfwSetCursorPosCallback` entrega coordenadas de TELA da área de conteúdo;
    o contexto RmlUi do `App` é dimensionado pelo tamanho de FRAMEBUFFER
    (`glfwGetFramebufferSize`, ver `app.cpp`/AUD-PUB-1). Sob HiDPI os dois diferem: aplicar
    escala `fb/win` (obtida via `glfwGetWindowSize` + `glfwGetFramebufferSize`) antes de
    emitir `MouseMove`. Sob Xvfb/X11 a escala é 1:1 (no-op), mas a fórmula existe e É TESTADA
    (ver seção 5 -- classe "fórmula de conversão sem teste de delta").
  - **Scroll:** **INVERSÃO DE SINAL OBRIGATÓRIA.** O yoffset positivo do GLFW é "rolar pra
    longe do usuário"; a convenção do `UiEvent::MouseWheel` (espelho do
    `Rml::Context::ProcessMouseWheel`) é y positivo rola pra BAIXO. O backend GLFW do próprio
    RmlUi nega os dois eixos. O implementer DEVE confirmar contra o source pinado do RmlUi
    (`build/_deps/rmlui-src/Backends/RmlUi_Backend_GLFW_*.cpp`) e citar a linha no
    doc-comment. Emitir `(-xoffset, -yoffset)`. Este é exatamente o tipo de fórmula que a
    casa exige coberta por teste de delta (memória `feedback_cadencia_releases_consumidor`).
  - **Char:** codepoint `unsigned int` -> encode UTF-8 (1-4 bytes) em buffer local ->
    `UiEvent::Type::Text` (lifetime >= chamada é satisfeito: o despacho é síncrono).
- Nota de hover (contrato já documentado em `ui_event.hpp`): o wheel do RmlUi rola o elemento
  sob HOVER; como o cursor callback emite `MouseMove` continuamente, o hover fica correto de
  graça no standalone -- registrar no doc-comment, não precisa de código extra.

### 2.3 Fiação + contrato público no `App` (módulo app)

Novo em `glintfx/include/glintfx/app.hpp` (nenhum tipo GLFW/RmlUi; só `<glintfx/ui_event.hpp>`,
header próprio e neutro):

```cpp
// EN: Inject a synthetic input event -- SAME neutral route the embed mode uses.
//     Physical window input arrives automatically (GLFW callbacks); this is the
//     synthetic/replay/test channel, parity with UiLayer::process_event.
//     Resize is a documented no-op here: the window is the source of truth for
//     size in App (AUD-PUB-1 sync_viewport machinery).
void process_event(const UiEvent& ev);

// EN: Game frame hook. Called once per render()/run() iteration with the GL
//     context current, AFTER the frame clear and BEFORE the UI render pass,
//     so the game scene composes UNDER the UI. dt_seconds = time since the
//     previous frame hook call (0.0 on the first call). Null callback = no-op.
void set_frame_callback(std::function<void(float dt_seconds)> cb);
```

- `App::App`: após `engine.attach()` com sucesso, instala
  `window.set_event_sink([impl](const UiEvent& ev){ impl->engine.process_event(ev, 0, 0); })`.
  O sink verifica `ok` (callbacks disparam dentro do `glfwPollEvents()` do `poll_events()`).
- `App::process_event(ev)`: guard `!impl_->ok` -> no-op; `Resize` -> no-op documentado;
  demais -> `engine.process_event(ev, 0, 0)`.
- **Frame hook:** `Engine::render_standalone` ganha ponto de inserção pro jogo desenhar
  DEPOIS do clear e ANTES do passe de UI (overload ou parâmetro
  `const std::function<void()>& scene_hook = {}`; o implementer escolhe lendo
  `render_gl3.cpp`). `App::render()` mede `dt` (clock GLFW via `SystemInterface_GLFW` ou
  `glfwGetTime` dentro de `app.cpp`, que já é TU GLFW-only) e invoca o callback do usuário
  dentro do hook. **Risco a verificar no código:** estado GL que o jogo deixar sujo não pode
  quebrar o passe de UI -- conferir se o caminho standalone do `RenderGl3` (re)estabelece o
  estado de que precisa por frame; se não, defender (reset mínimo) e DOCUMENTAR o contrato
  do hook ("pode deixar estado GL arbitrário" ou a lista do que não pode tocar).
- `run()` já vira o loop do jogo de graça (`poll -> update -> render`, com o hook dentro do
  `render`); nenhuma mudança de assinatura.

### 2.4 O que este A1 NÃO faz (anti-scope-creep)

- Gamepad (A2), áudio (A3), API de polling de input pro jogo além do que o RmlUi consome
  (só se o GusWorld pedir; semente de INBOX), sprite/batch API, flag formal
  `GLINTFX_MODULE_INPUT` (chega com A0 -- o código nasce posicionado, seção 2.2/2.1).
- Embed intocado em comportamento: `UiLayer::process_event` mantém contrato byte-a-byte
  (é refactor de delegação); `docs/embed-integration.md` só ganha nota "App agora tem input
  próprio; nada muda pra hosts embed".

## 3. Arquivos a tocar

| Arquivo | Mudança |
|---|---|
| `glintfx/include/glintfx/app.hpp` | + `#include <glintfx/ui_event.hpp>`, + `process_event`, + `set_frame_callback` (doc-comments bilíngues no padrão da casa) |
| `glintfx/src/engine.hpp` / `engine.cpp` | + `process_event(ev, offx, offy)`; recebe `to_rml_key`/`to_rml_mods`; + hook de cena no `render_standalone` |
| `glintfx/src/ui_layer.cpp` | `process_event` vira Resize-local + delegação ao Engine; helpers removidos daqui |
| `glintfx/src/window_glfw.hpp` / `.cpp` | + `set_event_sink`, user pointer, 5 callbacks + trampolins finos |
| `glintfx/src/glfw_event_translate.hpp` (novo) | funções puras de tradução GLFW -> `UiEvent` (testáveis sem janela) |
| `glintfx/src/app.cpp` | fiação do sink, `process_event`, `set_frame_callback` + dt |
| `glintfx/tests/CMakeLists.txt` + 3 testes novos | seção 5 |
| `CHANGELOG.md`, `README.md`, `TODO.md` | changelog da feature; item A1 -> `🔍 Pendente verificação`; citar `A1` no commit |

## 4. Gate de encapsulamento

- Superfície pública nova inclui SÓ headers próprios (`ui_event.hpp`) + `std::function`.
  Nenhum tipo GLFW/RmlUi em `glintfx/include/glintfx/` -- `tools/check_encapsulation.sh`
  deve seguir verde nos dois builds.
- `glfw_event_translate.hpp` e `window_glfw.hpp` são privados (`glintfx/src/`), nunca
  instalados.
- `UiEvent` segue o formato neutro único: NENHUM enum/constante GLFW vaza pra fora da TU de
  tradução.

## 5. Testes (TDD; nomes na convenção da suíte)

1. **`app_process_event_smoke.cpp`** (GLFW=ON, Xvfb): `App` + cena com botão clicável
   (reusar o padrão de `click_scene.rml/.rcss`); `set_click_callback`; injeta
   `MouseMove` + `MouseButton` down/up via `App::process_event`; assert do id clicado.
   Segunda perna: `set_focus(id)` + `Key::Enter` pressed/released -> click callback dispara
   (paridade com a rota embed já testada). Injeção de `Resize` -> no-op sem crash.
2. **`glfw_event_translate_sanity.cpp`** (unit, sem janela): exercita as funções puras --
   mapa de teclas completo incluindo `KP_ENTER` e descarte de tecla desconhecida;
   `GLFW_REPEAT -> pressed`; mapa explícito de mods; **inversão de sinal do scroll
   `(-x, -y)`**; **escala de cursor `fb/win`** com razão != 1 (o caso HiDPI que o Xvfb não
   exercita). Este teste é o dente contra a classe "fórmula de conversão sem delta testado".
3. **`app_frame_callback_smoke.cpp`** (GLFW=ON, Xvfb): `set_frame_callback` + N frames de
   `poll/update/render`; asserts: contador == N, `dt >= 0` e sano (< 10 s), primeiro `dt == 0`.
   Perna forte: dentro do hook, pintar uma região do framebuffer (scissor + clear color) e
   `snapshot()`; assert de pixel prova que a cena do jogo compõe SOB a UI e que o contexto GL
   está corrente no hook.
4. **Regressão:** suíte completa GLFW=ON + build/suíte embed GLFW=OFF (o refactor do
   `process_event` toca caminho compartilhado; o embed é o guarda de comportamento idêntico).

**Limitação honesta, registrada:** o GLFW não tem injeção sintética de evento de janela, então
o disparo FÍSICO dos trampolins (tecla real -> callback real) não é exercitável sob Xvfb sem
XTest (over-engineering pra esta fatia). Mitigação: trampolins de UMA linha sobre funções
puras 100% testadas + inspeção do reviewer + smoke manual nos demos existentes quando houver
sessão com display real. Registrar como known-limitation no doc-comment e na PR.

## 6. Ambiente de build/test (obrigatório)

```sh
export TMPDIR=/var/tmp   # tmpfs (/tmp) NAO: build pesado estoura RAM
cmake -S glintfx -B /var/tmp/glintfx-a1-build -DGLINTFX_BUILD_TESTS=ON
cmake --build /var/tmp/glintfx-a1-build -j
env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime \
  xvfb-run -a ctest --test-dir /var/tmp/glintfx-a1-build --output-on-failure
# depois, o build embed de regressao (UM build pesado por vez -- OOM):
cmake -S glintfx -B /var/tmp/glintfx-a1-embed -DGLINTFX_BUILD_TESTS=ON -DGLINTFX_BACKEND_GLFW=OFF
```

- **1 build pesado por vez** (restrição de OOM da máquina).
- Commits: Conventional Commits pt-br, citar **`A1`** no corpo, tocar `TODO.md` no mesmo
  commit (`🔍 Pendente verificação`, nunca `✅` direto). **NÃO push** (só o líder autoriza).

## 7. Agentes (até 2 sonnet, disciplina implementer != reviewer)

As superfícies do A1 se sobrepõem demais (`app.*` e `engine.*` aparecem nas três peças) pra
dois implementers paralelos. Divisão correta:

- **Agent 1 -- implementer (sonnet):** todo o A1 (seções 2-6), TDD red/green/refactor,
  na ordem: (i) refactor `Engine::process_event` + delegação do `UiLayer` (suíte embed verde
  antes de seguir); (ii) `glfw_event_translate` + teste unit; (iii) sink no `WindowGlfw` +
  fiação no `App` + `process_event` público + teste 1; (iv) frame callback + teste 3;
  (v) docs/CHANGELOG/TODO.
- **Agent 2 -- reviewer adversarial (sonnet, DIFERENTE do implementer):** EXECUTA, não só lê:
  builda os dois modos, roda a suíte, e mutation-testa as fórmulas (remover a inversão de
  sinal do scroll -> teste 2 TEM que falhar; remover a escala de cursor -> idem; remover o
  guard de não-finito movido pro Engine -> teste existente de hardening TEM que falhar);
  verifica o gate de encapsulamento; caça o gêmeo (auditoria-dominó: algum outro caminho
  novo faz cast de float sem validar? o hook de cena vaza estado GL que quebra o passe de
  UI? o sink dispara com `ok == false`?).

## 8. Verificação do orquestrador (checklist final, "relatório de agent não é prova")

1. Build limpo GLFW=ON **e** GLFW=OFF em `/var/tmp` (rodar, não aceitar claim).
2. Suíte completa verde nos dois modos sob Xvfb (contar os testes: os 3 novos presentes).
3. Rodar `app_process_event_smoke` isolado e conferir no output que o input sintético chegou
   à UI (id do click assertado).
4. `tools/check_encapsulation.sh` verde; grep manual: nenhum `GLFW`/`Rml` type em
   `glintfx/include/glintfx/app.hpp` além de comentário.
5. Spot-check das claims arquivo:linha do relatório do implementer (inversão de sinal citando
   o backend RmlUi pinado; trampolins de uma linha; Resize fora do Engine).
6. `git log` local conferido por conta própria: commit cita `A1`, `TODO.md` tocado, NENHUM
   push feito.
