# Plan WINDOW-MODES -- App window modes / Plano WINDOW-MODES -- modos de janela do App

- **Date / Data:** 2026-07-21
- **Author / Autor:** Caetano (CTO), for the orchestrator to execute with up to 4 sonnet agents
  (this slice needs only 3 -- see section 3)
- **Governs / Governa:** framework-2D App-mode slice WINDOW-MODES; target release **v0.16.0**
- **Origin / Origem:** consumer-driven (GusWorld, via bus): maximized window invades the KDE
  taskbar. Root cause is OURS: `glintfx::App` has no window-mode handling at all
  (`window_glfw.cpp` does a bare `glfwCreateWindow(w,h)`; `AppConfig` has only
  `width`/`height`). Table-stakes gap for a 2D game framework (SFML/Raylib/LÖVE all have it).
- **Non-negotiable constraints / Restrições inegociáveis:** App standalone ONLY --
  `UiLayer`/embed UNTOUCHED (the host owns the window there, ADR-0008);
  `windows-atoms.yml` UNTOUCHED; one heavy glintfx build at a time, build dirs under
  `/var/tmp` with `TMPDIR=/var/tmp`; implementer ≠ reviewer; NO push by agents.

---

## 1. Summary (EN, concise)

WINDOW-MODES adds a `WindowMode` enum (own type, zero third-party types in public headers)
to `AppConfig` **and** a runtime `App::set_window_mode()`, plus live queries
(`App::window_mode()`, `App::get_window_size()`). Four modes:

| Mode | Semantics | GLFW mechanism |
|---|---|---|
| `Windowed` | current behavior, `width`×`height`, decorated, movable | bare `glfwCreateWindow` (unchanged default) |
| `Maximized` | window maximized by the WM, **respecting the work area** (panels/taskbar visible) -- THE GusWorld fix | `glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE)` before create / `glfwMaximizeWindow` at runtime. The WM applies panel struts = work area FOR FREE; we never hand-size to the monitor |
| `FullscreenDesktop` | borderless fullscreen at the monitor's CURRENT video mode, covering EVERYTHING incl. panels (by design); no mode switch | create/`glfwSetWindowMonitor` with the primary monitor + `glfwGetVideoMode` dims + matching `GLFW_RED/GREEN/BLUE_BITS`/`GLFW_REFRESH_RATE` hints (GLFW's documented "windowed full screen" recipe) |
| `FullscreenExclusive` | real video-mode switch to the closest mode to `width`×`height` | create/`glfwSetWindowMonitor` with the primary monitor + the requested `w`,`h` (GLFW picks the closest mode) |

**Honest platform notes (documented, not hidden):** under native **Wayland**,
`FullscreenExclusive` is *emulated* by the compositor (GLFW documented behavior: no real
mode-setting; the compositor scales) -- it behaves like `FullscreenDesktop` with viewport
scaling; window position is not readable/settable (restore-to-windowed position is
best-effort). Under **X11/XWayland** all four modes are literal. Primary monitor only in
this slice; multi-monitor selection goes to INBOX.

**Resize/viewport:** free. The AUD-PUB-1 machinery (`Impl::sync_viewport`, called by BOTH
`render()` and `snapshot()`) re-reads the real framebuffer size every frame and re-flows
RmlUi on change -- mode changes propagate with zero new plumbing. One required refinement:
the App ctor currently seeds `Engine::attach()` with `cfg.width/height`; a window created
Maximized/Fullscreen has a DIFFERENT real framebuffer at frame 0, so the ctor must read
`window.size()` (real framebuffer) after `create()` and attach/seed with THAT (also fixes a
latent HiDPI first-frame mismatch). No framebuffer-size callback is needed.

**Testability split (headless vs manual):** under Xvfb there is NO window manager and NO
panel, so "maximize respects the work area" is **not machine-verifiable** (work area == full
screen; the maximize request may not even be honored without a WM). Headless tests prove:
each mode is accepted, the window is created per mode without crash, viewport/snapshot stay
sane and match `get_window_size()`, queries behave, transitions and hostile inputs never
crash. The REAL work-area/fullscreen behavior is a documented **manual smoke on a real
KDE/GNOME session**, run by the adversarial reviewer if a real display is available (same
pattern as the gamepad `/dev/input` smoke). The App test-suite N3 contract (ONE App per
process) forces the per-mode creation test to be ONE binary parameterized by environment
variable, registered 4x in ctest.

**Execution:** 3 agents. Wave 1: Agent A (`backend-engineer`, all code+tests) parallel with
Agent B (`technical-writer`, docs/CHANGELOG/TODO -- zero file overlap). Wave 2: Agent C
(`qa-engineer`, adversarial review that EXECUTES + the manual smoke). No CI workflow
changes; local static gate per slice: clang-tidy (`clang-analyzer-*`) + cppcheck
(compile_commands, `--error-exitcode=1`) + gitleaks. Embed-only build (GLFW=OFF) must keep
compiling -- all new window code lives under `GLINTFX_BACKEND_GLFW`.

---

## 2. Resumo e decisões (PT)

A fatia entrega o tratamento de modo de janela do `glintfx::App`: enum próprio `WindowMode`
no `AppConfig` (modo inicial) + `App::set_window_mode()` em runtime + queries
`App::window_mode()` / `App::get_window_size()`. O fix da dor do GusWorld é o
`Maximized` via `GLFW_MAXIMIZED` (hint pré-create ou `glfwMaximizeWindow`): o WM maximiza
respeitando os struts do painel (= work area) DE GRAÇA -- nunca dimensionamos pro monitor
inteiro à mão. `FullscreenDesktop` cobre tudo, inclusive painel, **by design** (é a
semântica de fullscreen borderless); quem quer respeitar a barra usa `Maximized`.

### 2.1 Decisões de engenharia (tomadas por mim; menores, seguir; ⚑ = confirmar retroativamente com o líder)

| # | Decisão | Racional |
|---|---|---|
| D1 | **Runtime switch ENTRA na fatia** (`set_window_mode`), não só o modo inicial no `AppConfig` | table-stakes de jogo (toggle fullscreen em menu de opções/Alt+Enter); o mecanismo GLFW (`glfwSetWindowMonitor`/`glfwMaximizeWindow`/`glfwRestoreWindow`) é o MESMO código do create -- adiar criaria API que muda depois. Custo marginal: rastrear geometria windowed pra restaurar. |
| D2 | **Multi-monitor NÃO entra** (primary monitor only) → INBOX | escopo mínimo defensável; expor `monitor_index` sem enumeração/query de monitores seria API manca; enumerar monitores é fatia própria. Documentado como limite honesto. |
| D3 | **Enum em header público próprio** `include/glintfx/window_mode.hpp` (padrão `element_box.hpp`/`font_engine.hpp`), incluído por `app.hpp`; SEM guard de módulo (enum puro, zero include de terceiro, inofensivo em build embed-only) | espelha o estilo da casa; `ui_layer.hpp` NÃO o inclui (embed intocado). |
| D4 | **Query `window_mode()` = estado VIVO derivado do GLFW**, não eco do último pedido: `glfwGetWindowMonitor()!=nullptr` → fullscreen (flag interna distingue Desktop×Exclusive, o GLFW não distingue); senão `glfwGetWindowAttrib(GLFW_MAXIMIZED)` → `Maximized`; senão `Windowed` | o usuário pode maximizar/restaurar pelo botão do WM sem passar pela API; eco do pedido mentiria. Consequência headless documentada: sob Xvfb (sem WM) o pedido `Maximized` pode não ser honrado e a query pode devolver `Windowed` -- o teste headless é tolerante nesse único caso (aceita ambos), o smoke manual prova o real. |
| D5 | **Fail-high da casa**: `set_window_mode` com enum inválido (cast fora de faixa) → `false` + no-op; `!ok()` → `false`; `AppConfig.window_mode` inválido no ctor → fallback `Windowed` documentado (create nunca falha por isso); `glfwGetPrimaryMonitor()==nullptr` (edge headless) → fallback windowed gracioso, nunca crash | mesma disciplina de `load(nullptr)`/`set_dp_ratio` (v0.3.0). |
| D6 | **Ctor attacha com o tamanho REAL do framebuffer** (ler `window.size()` após `create()` e usar em `Engine::attach` + seed de `last_render_w/h`), não mais `cfg.width/height` cru | janela criada Maximized/Fullscreen tem framebuffer ≠ cfg no frame 0; hoje o layout nasceria errado até o sync AUD-PUB-1 corrigir no 1º render. De quebra corrige o descasamento latente de HiDPI (framebuffer ≠ window size). Windowed comum: no-op (tamanhos iguais). |
| D7 | **Geometria de restauração**: antes de SAIR de `Windowed`, salvar pos+size (`glfwGetWindowPos/Size`); voltar a `Windowed` = `glfwSetWindowMonitor(win, nullptr, saved...)` (+`glfwRestoreWindow` se vinha de Maximized). Nunca esteve windowed (nasceu fullscreen): size = `cfg.width/height`, pos default `(64,64)` documentado. Wayland ignora posição (best-effort documentado) | comportamento padrão de todo framework de jogo; valores explícitos evitam janela nascendo sob o painel. |
| D8 | **Recipe GLFW canônica** por modo: Desktop = monitor + `glfwGetVideoMode` dims + hints `RED/GREEN/BLUE_BITS`+`REFRESH_RATE` do mode (sem troca de video mode); Exclusive = monitor + `cfg.w×h` (GLFW escolhe o mode mais próximo, troca real); Maximized = hint pré-create / `glfwMaximizeWindow` runtime; transição fullscreen→maximized = restaurar windowed primeiro, depois maximizar (ordem obrigatória) | é a receita documentada do GLFW pra "windowed full screen" vs exclusive; sob Xvfb (1 mode só) o "closest" degrada pro único -- sem crash. |
| D9 | **Sem callback de framebuffer-size** | o sync AUD-PUB-1 por-frame já é a autoridade (render() E snapshot()); callback duplicaria a fonte de verdade -- exatamente a classe de drift que o AUD-PUB-1-TWIN matou. |
| D10 | **Limitação Wayland documentada, não escondida**: `FullscreenExclusive` é EMULADO pelo compositor (sem mode-set real; comportamento GLFW documentado), posição de janela não é legível/gravável | a sessão do consumidor É Wayland/KDE; prometer troca de resolução real lá seria mentira de doc. |
| D11 | **Sem `resizable`/borderless-windowed/min-max size/callback de mode-change** nesta fatia → INBOX | cada um é API pública nova sem demanda de consumidor; anti over-engineering. |

### 2.2 Contrato da API pública

`include/glintfx/window_mode.hpp` (novo, Agente A -- SPDX + doc bilíngue, só `<cstdint>`):

```cpp
enum class WindowMode : std::uint8_t {
  Windowed,             // janela decorada width×height (default, comportamento atual)
  Maximized,            // maximizada pelo WM -- respeita a work area (painel visível)
  FullscreenDesktop,    // borderless no video mode atual do monitor -- cobre TUDO (by design)
  FullscreenExclusive,  // troca de video mode real pro mais próximo de width×height
};
```

`app.hpp` (Agente A edita):

```cpp
struct AppConfig {
  // ... campos existentes INTOCADOS ...
  // EN: Initial window mode. Invalid values fall back to Windowed (documented).
  //     Maximized respects the WM work area (panels stay visible); FullscreenDesktop
  //     covers everything including panels BY DESIGN; see window_mode.hpp + docs/window-modes.md.
  // PT: Modo inicial da janela. Valor inválido cai pra Windowed (documentado). ...
  WindowMode window_mode = WindowMode::Windowed;
};

class App {
  // EN: Switch window mode at runtime. Returns false (no-op) when !ok() or `mode` is not a
  //     valid enumerator; true when the request was issued to the window system (the WM/
  //     compositor has final say -- query window_mode() for the live result). Re-requesting
  //     the current mode is a safe no-op returning true. Wayland: FullscreenExclusive is
  //     compositor-emulated; restore position is best-effort. / PT: ...
  bool set_window_mode(WindowMode mode);

  // EN: Live window mode derived from the window system (NOT an echo of the last request):
  //     fullscreen flavor tracked internally, Maximized read from the WM, else Windowed.
  //     Returns Windowed when !ok(). Headless (no WM): a Maximized request may report
  //     Windowed -- documented. / PT: ...
  WindowMode window_mode() const;

  // EN: Current framebuffer size in physical pixels (the same size render()/snapshot()
  //     compose at). w=h=0 when !ok(). / PT: ...
  void get_window_size(int& w, int& h) const;
};
```

`window_glfw.hpp/.cpp` (interno, Agente A):

```cpp
bool create(const char* title, int w, int h, WindowMode mode);  // assinatura estendida
bool set_mode(WindowMode mode);   // runtime; false se win_ nulo/enum inválido
WindowMode mode() const;          // derivação D4; Windowed se win_ nulo
```

- `create()`: switch no `mode` ANTES do `glfwCreateWindow`: `Windowed` = caminho atual;
  `Maximized` = `glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE)` + create normal;
  `FullscreenDesktop`/`FullscreenExclusive` = `glfwGetPrimaryMonitor()` (nullptr → fallback
  windowed + `fprintf(stderr, ...)` uma vez, D5) + receita D8. Enum inválido → fallback
  `Windowed` (D5). Guarda `fullscreen_desktop_` flag interna quando aplicável (D4) e a
  geometria windowed inicial (D7).
- `set_mode()`: máquina de transição D7/D8. Todos os caminhos com null-check de `win_`
  (padrão do arquivo). Não toca nos 5 callbacks de input (registrados uma vez no create;
  `glfwSetWindowMonitor` NÃO os invalida -- mesma janela).
- `app.cpp`: ctor passa `cfg.window_mode` ao `create()` (com validação D5 ANTES, no App --
  o fallback é decidido uma vez, não em dois lugares); depois do create lê `window.size()`
  e usa o tamanho REAL no `Engine::attach` + seed `last_render_w/h` (D6). Os 3 métodos
  novos são forwards finos com guard `!ok()` (padrão do arquivo inteiro).

### 2.3 Testes (o que é provável headless × smoke manual)

**Headless-verificável (Xvfb, automatizado):**

1. `tests/app_window_mode_smoke.cpp` -- **UM binário, parametrizado por env var**
   `GLINTFX_TEST_WINDOW_MODE` ∈ {`windowed`,`maximized`,`fullscreen_desktop`,
   `fullscreen_exclusive`} (contrato N3 do App: UMA instância por processo -- 4 Apps num
   binário seria UB documentado). Registrado 4× no ctest
   (`app_window_mode_smoke_windowed` etc.) via
   `set_tests_properties(... PROPERTIES ENVIRONMENT "GLINTFX_TEST_WINDOW_MODE=...")` sobre o
   wrapper `run_xvfb.cmake` existente (zero mudança no wrapper). Cada corrida: construir App
   no modo, `ok()==true`, `load()` da cena compartilhada, 2× `poll+update+render`,
   `get_window_size()` > 0 em ambos os eixos, `snapshot()` true E dims do header PPM ==
   `get_window_size()` (prova viewport são), query `window_mode()`:
   - `windowed` → exige `Windowed`;
   - `fullscreen_desktop` → exige `FullscreenDesktop`; `fullscreen_exclusive` → exige
     `FullscreenExclusive` (fullscreen não depende de WM);
   - `maximized` → **tolerante**: aceita `Maximized` OU `Windowed` (sem WM o pedido pode não
     ser honrado -- D4), com comentário apontando pro smoke manual.
2. `tests/app_window_mode_transitions_smoke.cpp` -- um App (`Windowed` inicial), cicla:
   `Windowed→Maximized→Windowed→FullscreenDesktop→Windowed→FullscreenExclusive→Maximized→Windowed`,
   com `poll+update+render`+`get_window_size()` sã entre CADA transição (sem crash, sem
   framebuffer 0 persistente); re-pedir o modo corrente → true no-op; hardening no mesmo
   binário: `set_window_mode(static_cast<WindowMode>(99))` → false e `window_mode()`
   inalterado; `set_window_mode` após move (`ok()==false` na origem NÃO é chamável -- usar
   um App construído com `AppConfig` inválido de display? NÃO: basta o guard `!ok()` via o
   caminho normal ser coberto por inspeção + o teste de enum inválido; não fabricar App
   quebrado). `AppConfig.window_mode = static_cast<WindowMode>(255)` num processo à parte?
   NÃO (N3) -- o fallback de config inválida é coberto pelo sub-caso `windowed` do smoke 1
   com env var extra `GLINTFX_TEST_WINDOW_MODE=invalid_config` (5ª corrida registrada:
   constrói com enum lixo, exige `ok()==true` e `window_mode()==Windowed`).
3. Regressão: suíte COMPLETA verde (GLFW=ON) + **build embed-only GLFW=OFF compila e a
   suíte embed passa** (o código novo é todo gateado por `GLINTFX_BACKEND_GLFW`;
   `window_mode.hpp` é enum puro e inofensivo). `ui_layer.hpp`/`ui_layer.cpp`/
   `engine.cpp` com **diff ZERO** (prova de embed intocado -- reviewer confere no git).

**Smoke manual (sessão real KDE/GNOME -- reviewer executa se houver display; senão fica
documentado como pendência de verificação humana no relatório):**

- `Maximized`: janela maximizada NÃO cobre a barra de tarefas/painel (work area respeitada)
  -- é O critério do GusWorld;
- `FullscreenDesktop`: cobre tudo, painel inclusive; Alt-Tab sai são;
- `FullscreenExclusive` (X11): troca de mode real quando `w×h` ≠ desktop; (Wayland):
  comporta como desktop-fullscreen escalado -- CONFERIR que bate com a doc D10;
- transições runtime: fullscreen→windowed restaura tamanho/posição plausível; maximized→
  windowed restaura; nenhum frame preto persistente;
- procedimento: compilar `tests/app_window_mode_transitions_smoke` da build tree e rodar SEM
  Xvfb na sessão real (o binário de transições já é o harness; para o passo-a-passo visual,
  snippet scratch de 20 linhas em `/var/tmp` usando `set_frame_callback` + `set_window_mode`
  em timer -- NÃO commitado).

### 2.4 CMake

Agente A, `glintfx/tests/CMakeLists.txt`, DENTRO do bloco `if(GLINTFX_BACKEND_GLFW)`
existente (padrão `app_resize_smoke`): 2 executáveis novos, 6 entradas `add_test`
(4 modos + `invalid_config` + transições), todas via `run_xvfb.cmake` +
`set_tests_properties(... ENVIRONMENT ...)`. Nenhuma mudança em `glintfx/CMakeLists.txt`
além do **bump de versão do project() para 0.16.0** (lição da v0.3.0: `version()` travada
por bump esquecido -- conferir que o teste de versão passa).

### 2.5 Ambiente de execução (TODOS os agentes)

- Build pesado: dir em `/var/tmp` (ex.: `/var/tmp/gfx-winmode-<agente>`),
  `export TMPDIR=/var/tmp`. **UM build glintfx por vez na máquina.** NUNCA 2 simultâneos.
- Configure/​build/​suíte:
  `cmake -S glintfx -B /var/tmp/gfx-winmode-<ag> -DGLINTFX_BUILD_TESTS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build /var/tmp/gfx-winmode-<ag> -j$(nproc)`
  `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest --test-dir /var/tmp/gfx-winmode-<ag> --output-on-failure -j1`
- Build embed-only (prova GLFW=OFF; leve, mas conta como build -- serializar):
  `cmake -S glintfx -B /var/tmp/gfx-winmode-<ag>-embed -DGLINTFX_BUILD_TESTS=ON -DGLINTFX_BACKEND_GLFW=OFF ...` + suíte embed.
- Estática LOCAL obrigatória (lição v0.13/v0.14/v0.15): `clang-tidy -p <bld>
  glintfx/src/window_glfw.cpp glintfx/src/app.cpp` (gate = `clang-analyzer-*`, ver
  `.clang-tidy`); `cppcheck --project=<bld>/compile_commands.json --error-exitcode=1
  --file-filter='*src/window_glfw*' --file-filter='*src/app.cpp'`; **gitleaks** na árvore
  (mordeu na v0.15.0 -- rodar ANTES de dar por pronto).
- ASan da suíte completa fica com o gate `claudio`/nightly no push (não rodar 2 builds
  pesados locais).
- Commits: Conventional Commits pt-br, citar **`A4-WINMODES`** no corpo (se `A4` já estiver
  tomado no `TODO.md`, usar o próximo `A*` livre mantendo o sufixo `-WINMODES`), tocar o
  `Status` do item no mesmo commit (`🔍 Pendente verificação`, nunca `✅` direto).
  **NÃO fazer push** (só o orquestrador, na cadência de onda do modo autônomo).

---

## 3. Waves and file ownership / Ondas e posse de arquivos (disjunção)

Headcount calibrado: 1 arquivo de janela + fachada + testes + docs = **3 agentes**, não 4
(implementação é acoplada demais pra dividir sem colisão).

| Onda | Agente | Arquivos (posse EXCLUSIVA na onda) |
|---|---|---|
| 1 | **A** `backend-engineer` | `include/glintfx/window_mode.hpp` (novo), `include/glintfx/app.hpp`, `src/window_glfw.{hpp,cpp}`, `src/app.cpp`, `tests/app_window_mode_smoke.cpp`, `tests/app_window_mode_transitions_smoke.cpp`, **+ edits**: `glintfx/tests/CMakeLists.txt`, `glintfx/CMakeLists.txt` (só bump 0.16.0) |
| 1 (paralelo) | **B** `technical-writer` | `glintfx/docs/window-modes.md` (novo), `README.md` (linha de feature), `CHANGELOG.md` (seção v0.16.0), `TODO.md` (item `A4-WINMODES` 🔍 + INBOX D2/D11) |
| 2 | **C** `qa-engineer` (review adversarial) | read-only + execução (headless + smoke manual se houver display real); achados corrigidos por A via orquestrador |

Colisão A↔B: nenhuma (B não toca código nem CMake; B escreve a doc a partir da seção 2.2
deste plano, que é o contrato -- se o A divergir do plano, o reviewer C acusa e a doc é
corrigida na remediação). C só depois de A+B entregues e re-verificados pelo orquestrador.

---

## 4. CI plan / Plano de CI

- **Linux (github, gate primário):** zero mudança de workflow -- clang-tidy
  (`src/**/*.cpp`) e cppcheck via compile_commands pegam as TUs alteradas; a matriz
  GLFW=ON/OFF prova o embed-only; os testes novos entram no ctest normal (Xvfb).
- **`claudio` (heavy.yml, fedora:42):** sem mudança; ASan da suíte cobre a fatia no PR/tag.
- **Windows (`windows-atoms.yml`): INTOCADO.** App/janela não está no harness de átomos
  puros (`tests/win-atoms/` compila lista explícita); nenhum path novo casa os filtros.
  GLFW é cross-platform mas os testes desta fatia são Xvfb/Linux -- **não fabricar teste
  Windows**. (Reviewer confirma as 2 claims.)
- Verificação estática local por fatia (seção 2.5) é o gate ANTES do push do orquestrador.

---

## 5. Briefs (ready to paste / prontos pra colar)

### 5.1 Agente A -- `backend-engineer` -- enum + janela + fachada + testes (onda 1)

> Você é o backend-engineer da fatia WINDOW-MODES (A4-WINMODES) do glintfx (framework 2D).
> Leia antes: `glintfx/docs/superpowers/plans/2026-07-21-framework2d-window-modes.md`
> (este plano -- seções 2.1 a 2.5 são o SEU contrato), `glintfx/src/window_glfw.{hpp,cpp}`,
> `glintfx/src/app.cpp` + `glintfx/include/glintfx/app.hpp` (atenção ao contrato N3 "um App
> por processo" e ao AUD-PUB-1 `sync_viewport`), `glintfx/tests/app_resize_smoke.cpp` +
> o registro dele em `tests/CMakeLists.txt` (MODELO de teste GLFW/Xvfb), `AGENTS.md`
> (gotchas + convenções). NÃO toque em `ui_layer.*`, `engine.*`, `docs/`, `README.md`,
> `CHANGELOG.md`, `TODO.md` (posse do agente B / fora da fatia).
>
> **Entregue (posse exclusiva):**
> 1. `glintfx/include/glintfx/window_mode.hpp` -- `enum class WindowMode : std::uint8_t`
>    (seção 2.2, literal), SPDX, doc-comment bilíngue (en primeiro) com a semântica de CADA
>    modo, incluindo: Maximized respeita work area do WM; FullscreenDesktop cobre painéis BY
>    DESIGN; FullscreenExclusive é emulado sob Wayland (D10). Só `<cstdint>`, zero tipo de
>    terceiro, sem guard de módulo (D3).
> 2. `glintfx/src/window_glfw.{hpp,cpp}` -- `create(title, w, h, WindowMode)` com o switch
>    pré-create (D8: hint GLFW_MAXIMIZED; monitor+videomode+hints RED/GREEN/BLUE_BITS+
>    REFRESH_RATE pra Desktop; monitor+w×h pra Exclusive), fallback windowed gracioso quando
>    `glfwGetPrimaryMonitor()==nullptr` (stderr uma vez, D5); `set_mode(WindowMode)` com a
>    máquina de transição D7 (salvar pos/size ao sair de Windowed; voltar =
>    `glfwSetWindowMonitor(win, nullptr, saved…)`; fullscreen→maximized = restaurar
>    windowed PRIMEIRO, depois `glfwMaximizeWindow`; defaults `(64,64)`/cfg quando nunca foi
>    windowed); `mode()` com a derivação viva D4 (`glfwGetWindowMonitor` + flag interna
>    Desktop×Exclusive + `glfwGetWindowAttrib(GLFW_MAXIMIZED)`). Null-check de `win_` em
>    tudo (padrão do arquivo). NÃO mexa nos 5 callbacks de input.
> 3. `glintfx/include/glintfx/app.hpp` + `glintfx/src/app.cpp` -- campo
>    `AppConfig::window_mode` (default `Windowed`; valor inválido validado NO APP antes do
>    create → fallback Windowed documentado, D5) + `set_window_mode()` / `window_mode()` /
>    `get_window_size()` (contratos da seção 2.2, doc-comments bilíngues completos, guards
>    `!ok()` no padrão do arquivo). **D6 obrigatório:** após `window.create()`, leia
>    `window.size()` e use o tamanho REAL no `Engine::attach` e no seed de
>    `last_render_w/h` (doc-comment explicando o porquê -- janela maximizada/fullscreen tem
>    framebuffer ≠ cfg no frame 0).
> 4. Testes (seção 2.3, literal): `tests/app_window_mode_smoke.cpp` (parametrizado por
>    `GLINTFX_TEST_WINDOW_MODE`, 5 corridas: 4 modos + `invalid_config`; asserts da 2.3
>    incluindo PPM-dims == `get_window_size()` e a tolerância DOCUMENTADA do caso
>    `maximized` headless) e `tests/app_window_mode_transitions_smoke.cpp` (ciclo completo de
>    transições + re-pedido no-op + enum inválido → false). Registro em
>    `tests/CMakeLists.txt` DENTRO do bloco `if(GLINTFX_BACKEND_GLFW)`, 6 `add_test` via
>    `run_xvfb.cmake` + `set_tests_properties(... PROPERTIES ENVIRONMENT ...)` (NÃO mude o
>    `run_xvfb.cmake`). Estilo dos testes existentes: asserts com mensagem, exit != 0 em
>    falha, TDD red/green/refactor.
> 5. `glintfx/CMakeLists.txt`: bump da versão do `project()` para **0.16.0** (e só isso).
> 6. Verificação local ANTES de reportar (única janela de build pesado da máquina; dirs em
>    `/var/tmp`, `export TMPDIR=/var/tmp`): build GLFW=ON + suíte COMPLETA sob o wrapper
>    Xvfb (linhas exatas na seção 2.5) -- zero regressão; DEPOIS build embed-only
>    GLFW=OFF + suíte embed (prova de compilação e verde); clang-tidy + cppcheck +
>    **gitleaks** (seção 2.5). Confirme `git diff --stat` NÃO toca `ui_layer.*`/`engine.*`.
> 7. Commit local (Conventional Commits pt-br, citar `A4-WINMODES`), **NÃO push**.
>    Convenções: SPDX em todo arquivo de código; identificadores en-intl; doc-comments
>    bilíngues. Reporte: arquivos, saída dos ctests (GLFW=ON e OFF), decisões locais,
>    limitações honestas (ex.: o que o Xvfb não prova).

### 5.2 Agente B -- `technical-writer` -- docs + changelog + TODO (onda 1, paralelo a A)

> Você é o technical-writer da fatia WINDOW-MODES (A4-WINMODES) do glintfx. Leia antes:
> `glintfx/docs/superpowers/plans/2026-07-21-framework2d-window-modes.md` (o contrato da
> API está na seção 2.2; semântica e limites nas 2.1/2.3), `glintfx/docs/gamepad.md` ou
> `docs/effects.md` (MODELO de doc bilíngue da casa), `README.md`, `CHANGELOG.md`,
> `TODO.md`. **NÃO toque em código, headers, testes nem CMake** (posse do agente A).
> A doc segue o PLANO como fonte de verdade; divergência plano↔código é achado do reviewer,
> não seu.
>
> **Entregue (posse exclusiva):**
> 1. `glintfx/docs/window-modes.md` bilíngue (en primeiro, pt depois, MESMO arquivo):
>    os 4 modos com semântica exata (tabela da seção 1 do plano), exemplo de uso
>    (`AppConfig.window_mode` + `set_window_mode` num menu de opções via
>    `set_frame_callback`), a distinção **Maximized (respeita work area / barra visível) ×
>    FullscreenDesktop (cobre tudo by design)** em destaque -- é a dúvida que originou a
>    fatia --, queries (`window_mode()` vivo, `get_window_size()`), fail-high (enum
>    inválido, pré-init), **notas de plataforma honestas**: Wayland (Exclusive emulado,
>    posição best-effort -- D10), headless/Xvfb (Maximized pode não ser honrado sem WM),
>    primary-monitor-only (D2), e a seção "verificação manual" com o roteiro do smoke da
>    seção 2.3 (pra qualquer humano reproduzir).
> 2. `README.md`: 1 linha na lista de features do App apontando pra doc nova.
> 3. `CHANGELOG.md`: seção v0.16.0 (A4-WINMODES) no padrão das anteriores (consumer-driven
>    GusWorld: maximize respeitando work area; 4 modos; runtime switch; queries; D6).
> 4. `TODO.md`: item `A4-WINMODES` com status `🔍 Pendente verificação` (se o ID `A4` já
>    existir, use o próximo `A*` livre com sufixo `-WINMODES` e cite no relatório); itens
>    INBOX: multi-monitor/`monitor_index` (D2), `resizable`/borderless-windowed/min-max
>    size/callback de mudança de modo (D11), encaminhamento de wheel no App (lacuna
>    pré-existente citada no CLAUDE.md, se ainda não estiver lá).
> 5. Commit local (Conventional Commits pt-br, citar `A4-WINMODES`), **NÃO push**. Sem
>    build pesado (a janela de build é do agente A). Rode **gitleaks** na árvore antes de
>    reportar.

### 5.3 Agente C -- `qa-engineer` -- review adversarial QUE EXECUTA (onda 2)

> Você é o reviewer adversarial da fatia WINDOW-MODES (implementer ≠ reviewer; você NÃO
> escreveu nada disso). Leia o plano
> `glintfx/docs/superpowers/plans/2026-07-21-framework2d-window-modes.md` inteiro e o diff
> completo da fatia (`git log`/`git diff` dos commits A4-WINMODES). Sua revisão EXECUTA --
> relatório sem execução será rejeitado. Build em `/var/tmp/gfx-winmode-rev`,
> `TMPDIR=/var/tmp`, UM build pesado por vez; se OOM, reporte em vez de thrashear.
>
> **Execute, no mínimo:**
> 1. **Rebuild limpo + suíte completa** (GLFW=ON, wrapper Xvfb, seção 2.5) -- zero falha,
>    zero regressão. Depois **build embed-only GLFW=OFF** + suíte embed verde.
> 2. **Embed intocado:** `git diff <base>..HEAD -- glintfx/src/ui_layer.* glintfx/src/engine.*
>    glintfx/include/glintfx/ui_layer.hpp` == vazio. Confirme que `window_mode.hpp` não é
>    incluído por nenhum header do embed.
> 3. **Encapsulamento:** `./tools/check_encapsulation.sh glintfx/include/glintfx`; grep
>    manual: zero tipo GLFW em `window_mode.hpp`/`app.hpp`.
> 4. **Adversarial de API (escreva harness scratch, não só os fixtures):**
>    `set_window_mode(static_cast<WindowMode>(4))`, `(255)`, enum válido repetido 100× em
>    loop com render entre eles (leak/estado GL? rode sob a suíte, observe RSS),
>    transições em TODAS as ordens de pares (Windowed↔Maximized↔Desktop↔Exclusive),
>    `set_window_mode` ANTES de `load()` e DEPOIS de janela fechada; `get_window_size` em
>    cada estado. `AppConfig{.width=-5, .window_mode=FullscreenExclusive}` -- comportamento
>    documentado (create falha limpo OU degrada, mas NUNCA crash)?
> 5. **Mutation-test (mínimo 4 mutantes, à mão):** (a) troque o hint GLFW_MAXIMIZED por
>    no-op -- algum teste morre? (headless: talvez NÃO morra -- se sobreviver, é o gap
>    Xvfb documentado; confirme que a doc/smoke manual o cobre e reporte como risco aceito,
>    não como pass); (b) inverta Desktop×Exclusive na flag interna -- a query nos testes
>    mata?; (c) remova o D6 (attach com cfg em vez do framebuffer real) -- o assert
>    PPM==get_window_size mata?; (d) quebre a restauração de geometria (não salvar size) --
>    o ciclo de transições mata? Mutante sobrevivente sem justificativa = achado.
> 6. **Domino check (auditoria-dominó da casa):** todo guard novo -- procure o gêmeo
>    (`set_mode` valida enum; `create` também? `mode()` com `win_` nulo? fallback de
>    monitor nulo existe nos DOIS call sites -- create E set_mode?). Varra a superfície.
> 7. **Smoke manual (se houver display real na máquina -- sessão KDE/Wayland do líder):**
>    rode o roteiro da seção 2.3 (binário de transições SEM Xvfb + snippet scratch em
>    `/var/tmp`): maximize NÃO cobre a barra de tarefas (critério GusWorld); Desktop cobre
>    tudo; Exclusive comporta conforme doc D10 no Wayland; restaurações sãs. A janela vai
>    aparecer na tela do líder -- rode uma vez, rápido, e feche. Se não houver display,
>    declare o smoke como PENDENTE DE VERIFICAÇÃO HUMANA no veredito (não invente).
> 8. **Estática + claims:** clang-tidy + cppcheck + gitleaks (seção 2.5); confirme as 2
>    claims de CI (path-filters do `windows-atoms.yml`, harness win-atoms sem App);
>    spot-check >= 5 claims arquivo:linha dos relatórios de A/B; confirme o bump 0.16.0 e
>    que a doc do agente B bate com o CÓDIGO real (não só com o plano).
>
> Classifique achados (CRITICAL/MAJOR/MINOR/NIT) com repro mínimo executável para cada
> CRITICAL/MAJOR. **NÃO corrija você mesmo; NÃO push.** Veredito:
> APROVADO / APROVADO-COM-RESSALVAS / REPROVADO (+ status do smoke manual).

---

## 6. Risks / Riscos

1. **"Maximize respeita work area" não é provável headless** (Xvfb sem WM/painel) -- o
   risco nº 1 da fatia é entregar o fix sem prova automatizada. Mitigação: query viva D4 +
   smoke manual obrigatório no review (5.3 item 7) + doc com roteiro reproduzível; o
   mutante (a) do item 5 do review expõe o gap honestamente em vez de fingir cobertura.
2. **Divergência Wayland×X11** (Exclusive emulado, posição não-suportada) -- mitigada por
   D10 (documentar, não prometer) + smoke na sessão Wayland real do líder.
3. **Maximize assíncrono** (WM aplica depois do create) → 1º frame em tamanho velho --
   mitigado por D6 (attach com framebuffer real) + AUD-PUB-1 que corrige no frame seguinte
   de qualquer jeito (cinto e suspensório já existentes).
4. **Contrato N3 (um App/processo) × testes por modo** -- resolvido por env-var + 5
   registros ctest (2.3); se um agente ignorar isso e criar N Apps num binário, é UB
   documentado -- reviewer deve conferir.
5. **Geometria de restauração** (heurística D7, posição best-effort no Wayland) -- risco
   cosmético aceito e documentado; nunca crash.
6. **OOM local**: 3 builds necessários (GLFW=ON, GLFW=OFF, review) -- SEMPRE serializados,
   dirs `/var/tmp`, ASan completo fica no gate `claudio`.

## 7. Scope recommendation / Recomendação de escopo (fatia mínima defensável)

**Dentro:** 4 modos no `AppConfig` + `set_window_mode` runtime (D1) + queries
`window_mode()`/`get_window_size()` + D6 (attach com framebuffer real) + docs bilíngues +
smoke manual documentado. **Fora (INBOX):** multi-monitor/`monitor_index` (D2),
`resizable`, borderless-windowed (janela sem borda NÃO-fullscreen), min/max size,
callback de mudança de modo, vsync/refresh-rate control (D11), encaminhamento de wheel no
App (lacuna pré-existente, não desta fatia).

## 8. Open questions for the leader / Perguntas ao líder (nenhuma bloqueia o início)

1. ⚑ D1: confirmar retroativamente a inclusão do runtime switch (`set_window_mode`) na
   fatia (alternativa mais magra era só o modo inicial no `AppConfig`).
2. ⚑ D2: confirmar multi-monitor como INBOX (primary-only nesta fatia).
