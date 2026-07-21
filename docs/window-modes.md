# Window modes (A4-WINMODES reference and how-to)

> **EN:** `glintfx::App` window-mode API reference: the four `WindowMode` values, the
> `AppConfig::window_mode`/`App::set_window_mode()` contract, the live queries, and the honest
> platform limits (Wayland, headless/Xvfb, primary-monitor-only). Diátaxis type: **how-to /
> reference**. Audience: a developer using the standalone `glintfx::App` who needs a maximize
> that respects the desktop panel, a fullscreen toggle, or a runtime mode switch bound to an
> options menu.
> **PT:** Referência da API de modo de janela do `glintfx::App`: os quatro valores de
> `WindowMode`, o contrato `AppConfig::window_mode`/`App::set_window_mode()`, as queries vivas, e
> os limites honestos de plataforma (Wayland, headless/Xvfb, primary-monitor-only). Tipo
> Diátaxis: **how-to / reference**. Audiência: um dev usando o `glintfx::App` standalone que
> precisa de um maximizar que respeita o painel do desktop, um toggle de fullscreen, ou uma troca
> de modo em runtime ligada a um menu de opções.

This is `App`-only (standalone). `glintfx::UiLayer` (embed/guest mode) is untouched -- the host
owns the window there ([ADR-0008](adr/0008-embed-guest-mode.md)); window modes make no sense for
a library that doesn't create a window.

**Origin:** consumer-driven ([GusWorld](https://codeberg.org/petrinhu/glintfx), via the
cross-project bus). Maximizing the window used to invade the KDE taskbar -- the root cause was
ours: `glintfx::App` had no window-mode handling at all, `AppConfig` had only `width`/`height`.
Table-stakes gap for a 2D game framework (SFML/Raylib/LÖVE all have it).

---

## English

### The four modes

| Mode | Semantics | Panel/taskbar |
| :--- | :--- | :--- |
| `Windowed` | Current behavior: `width`×`height`, decorated, movable. Default. | Visible (normal window) |
| `Maximized` | Maximized **by the window manager**, which applies its own panel struts. | **Respected -- the fix.** |
| `FullscreenDesktop` | Borderless fullscreen at the monitor's *current* video mode. No mode switch. | **Covered, by design.** |
| `FullscreenExclusive` | Real video-mode switch to the mode closest to `width`×`height`. | Covered (mode-dependent) |

**The distinction that started this doc:** `Maximized` and `FullscreenDesktop` are *not*
interchangeable. `Maximized` hands sizing to the window manager, which already knows where its
own panels/taskbar are and leaves them visible -- this is the GusWorld fix, and it costs
`glintfx` nothing to get right, because the WM does the work. `FullscreenDesktop` covers
*everything*, panel included -- that is the correct, expected behavior of a borderless-fullscreen
mode, not a bug. If your game wants "big window that respects the desktop", use `Maximized`. If
it wants "immersive, nothing else visible", use `FullscreenDesktop` or `FullscreenExclusive`.

### Usage

**Initial mode, at boot:**

```cpp
glintfx::App app({
    .title = "my game",
    .width = 1280, .height = 720,
    .window_mode = glintfx::WindowMode::Maximized,   // respects the WM's panel from frame 0
});
app.load("menu.rml");
app.run();
```

An invalid `window_mode` value (e.g. a stray cast) falls back to `Windowed` -- `create()` never
fails because of this field.

**Runtime switch,** e.g. wired to an options-menu button or a fullscreen-toggle shortcut:

```cpp
app.set_click_callback([&app](const char* id) {
    if (std::string_view(id) == "btn-fullscreen") {
        app.set_window_mode(glintfx::WindowMode::FullscreenDesktop);
    } else if (std::string_view(id) == "btn-windowed") {
        app.set_window_mode(glintfx::WindowMode::Windowed);
    }
});
```

`set_window_mode` also composes naturally with a per-frame hook
(`App::set_frame_callback(fn(dt))`, [A1-INPUT](../CHANGELOG.md)) if your game drives the toggle
from its own input/state machine instead of a UI click -- the call itself is the same either way.
Re-requesting the mode `App` is already in is a safe no-op (returns `true`, no visible flicker).

**Live queries, e.g. to reflect the current mode in an options menu:**

```cpp
glintfx::WindowMode mode = app.window_mode();   // live, from the window system -- see below
int w = 0, h = 0;
app.get_window_size(w, h);                      // physical framebuffer pixels
```

### Why `window_mode()` is not an echo of your last request

`window_mode()` derives the answer **from the window system itself** (`glfwGetWindowMonitor` for
the fullscreen flavors, `glfwGetWindowAttrib(GLFW_MAXIMIZED)` for `Maximized`), not from a stored
copy of the last value you passed to `set_window_mode`. This matters because the user can
maximize or restore the window with the WM's own title-bar button, entirely outside your code --
an echo of the last API call would then lie. The one honest cost: on a machine with **no window
manager** (Xvfb, some CI/headless setups) a `Maximized` request may not be honored at all, and the
live query then reports `Windowed` -- see "Headless / Xvfb" below.

### Fail-high

- `set_window_mode(mode)` returns `false` (no-op, current mode unchanged) when the `App` is
  `!ok()`, or when `mode` is not one of the four valid enumerators (e.g. a raw cast from an
  out-of-range integer). It never crashes on bad input.
- `AppConfig::window_mode` set to an invalid enumerator at construction falls back to `Windowed`,
  documented, never a failed `create()`.
- `get_window_size` returns `w = h = 0` when `!ok()`.

### Platform notes (honest, not hidden)

- **Wayland:** `FullscreenExclusive` is *emulated* by the compositor -- there is no real
  video-mode switch (this is GLFW's own documented behavior on Wayland, not a glintfx
  limitation); it behaves like `FullscreenDesktop` with the compositor scaling the surface.
  Restoring a window's position after leaving fullscreen/maximized is **best-effort**: Wayland
  does not let a client read or set its own window position, so a window that started fullscreen
  and returns to `Windowed` lands at a documented default rather than "where it was".
- **X11 / XWayland:** all four modes are literal -- `FullscreenExclusive` does a real mode switch,
  window position is readable/settable, restoration is exact.
- **Headless / Xvfb:** there is no window manager and no panel under Xvfb, so "maximize respects
  the work area" is **not machine-verifiable** there -- the maximize request may not even be
  honored without a WM, and `window_mode()` may then report `Windowed` for a `Maximized` request.
  This is expected, not a bug; it is why this slice's headless test suite treats that one case as
  tolerant (accepts either answer) and the real behavior is proven by the manual smoke below, on
  a real desktop session.
- **Primary monitor only.** This slice always targets `glfwGetPrimaryMonitor()`; there is no
  `monitor_index` or monitor-enumeration API yet. See "Out of scope" below.

### API reference

| Symbol | Notes |
| :--- | :--- |
| `WindowMode` (enum class) | `Windowed`, `Maximized`, `FullscreenDesktop`, `FullscreenExclusive`. `glintfx/include/glintfx/window_mode.hpp`. |
| `AppConfig::window_mode` | Initial mode. Default `Windowed`. Invalid value -> `Windowed` (documented, never fails `create()`). |
| `App::set_window_mode(WindowMode mode) -> bool` | Runtime switch. `false` (no-op) when `!ok()` or `mode` is invalid; `true` when the request was issued to the window system (the WM/compositor has final say -- query `window_mode()` for the live result). Re-requesting the current mode is a safe no-op returning `true`. |
| `App::window_mode() const -> WindowMode` | Live, derived from the window system, not an echo of the last request. `Windowed` when `!ok()`. |
| `App::get_window_size(int& w, int& h) const -> void` | Current framebuffer size in physical pixels -- the same size `render()`/`snapshot()` compose at. `w = h = 0` when `!ok()`. |

### Out of scope (this slice)

- **Multi-monitor / `monitor_index`.** Primary monitor only; exposing an index without a
  monitor-enumeration API would be a half-API. A future slice if a real consumer needs picking a
  specific display.
- **`resizable`, borderless-windowed (a non-fullscreen undecorated window), min/max window size,
  a mode-change callback, vsync/refresh-rate control.** None has a confirmed consumer yet; each
  would be new public surface on its own.
- **Mouse-wheel forwarding in `App`.** A separate, pre-existing gap (`App` has no wheel path at
  all, `UiLayer` has had it since v0.4.0) -- unrelated to window modes, not touched by this slice.

### Manual verification (a real desktop session)

Xvfb has no window manager and no panel, so the one claim this doc makes that actually matters --
"maximize respects the taskbar" -- cannot be proven by the automated suite. Reproduce it yourself
on a real KDE/GNOME (or other WM) session:

1. Build the test suite (`-DGLINTFX_BUILD_TESTS=ON`) and run
   `tests/app_window_mode_transitions_smoke` directly, **without** the Xvfb wrapper, on your real
   session. It cycles `Windowed -> Maximized -> Windowed -> FullscreenDesktop -> Windowed ->
   FullscreenExclusive -> Maximized -> Windowed`, so you get all four modes and their transitions
   in one run -- watch the window rather than reading test output.
2. **`Maximized`:** the window grows to fill the screen but the taskbar/panel stays visible and
   un-covered. This is the GusWorld criterion.
3. **`FullscreenDesktop`:** the window covers everything, panel included -- by design. Alt-Tab
   away and back cleanly.
4. **`FullscreenExclusive`:** on X11, a real mode switch happens when `width`×`height` differs
   from the desktop resolution; on Wayland, it behaves like `FullscreenDesktop` (scaled) -- confirm
   this matches the platform note above, don't expect a literal resolution change there.
5. Watch every transition: no persistent black frame, and returning to `Windowed` from fullscreen
   or maximized lands at a plausible size/position (exact position is best-effort on Wayland, per
   the note above).

---

## Português

### Os quatro modos

| Modo | Semântica | Painel/barra de tarefas |
| :--- | :--- | :--- |
| `Windowed` | Comportamento atual: `width`×`height`, decorada, móvel. Default. | Visível (janela normal) |
| `Maximized` | Maximizada **pelo window manager**, que aplica os próprios struts de painel. | **Respeitado -- o fix.** |
| `FullscreenDesktop` | Borderless fullscreen no video mode *atual* do monitor. Sem troca de modo. | **Coberto, by design.** |
| `FullscreenExclusive` | Troca de video mode real pro modo mais próximo de `width`×`height`. | Coberto (depende do modo) |

**A distinção que originou este doc:** `Maximized` e `FullscreenDesktop` **não** são
intercambiáveis. `Maximized` entrega o dimensionamento ao window manager, que já sabe onde ficam
os próprios painéis/barra de tarefas e os deixa visíveis -- é o fix do GusWorld, e não custa nada
pro `glintfx` acertar, porque o WM faz o trabalho. `FullscreenDesktop` cobre *tudo*, painel
incluso -- esse é o comportamento correto e esperado de um modo borderless-fullscreen, não um bug.
Se seu jogo quer "janela grande que respeita o desktop", use `Maximized`. Se quer "imersivo, nada
mais visível", use `FullscreenDesktop` ou `FullscreenExclusive`.

### Uso

**Modo inicial, no boot:**

```cpp
glintfx::App app({
    .title = "meu jogo",
    .width = 1280, .height = 720,
    .window_mode = glintfx::WindowMode::Maximized,   // respeita o painel do WM desde o frame 0
});
app.load("menu.rml");
app.run();
```

Um valor inválido de `window_mode` (ex.: um cast perdido) cai pra `Windowed` -- o `create()` nunca
falha por causa deste campo.

**Troca em runtime,** ex.: ligada a um botão de menu de opções ou atalho de toggle de fullscreen:

```cpp
app.set_click_callback([&app](const char* id) {
    if (std::string_view(id) == "btn-fullscreen") {
        app.set_window_mode(glintfx::WindowMode::FullscreenDesktop);
    } else if (std::string_view(id) == "btn-windowed") {
        app.set_window_mode(glintfx::WindowMode::Windowed);
    }
});
```

`set_window_mode` também compõe naturalmente com um hook por-frame
(`App::set_frame_callback(fn(dt))`, [A1-INPUT](../CHANGELOG.md)) se seu jogo dirigir o toggle pela
própria máquina de estado/input em vez de um clique de UI -- a chamada em si é a mesma nos dois
casos. Repedir o modo em que o `App` já está é um no-op seguro (retorna `true`, sem flicker
visível).

**Queries vivas, ex.: pra refletir o modo atual num menu de opções:**

```cpp
glintfx::WindowMode mode = app.window_mode();   // vivo, do window system -- ver abaixo
int w = 0, h = 0;
app.get_window_size(w, h);                      // pixels físicos do framebuffer
```

### Por que `window_mode()` não é um eco do seu último pedido

`window_mode()` deriva a resposta **do próprio window system** (`glfwGetWindowMonitor` pros
sabores fullscreen, `glfwGetWindowAttrib(GLFW_MAXIMIZED)` pro `Maximized`), não de uma cópia
guardada do último valor que você passou pro `set_window_mode`. Isso importa porque o usuário pode
maximizar ou restaurar a janela pelo próprio botão da barra de título do WM, totalmente fora do
seu código -- um eco do último request mentiria nesse caso. O único custo honesto: numa máquina
**sem window manager** (Xvfb, alguns setups de CI/headless) um pedido de `Maximized` pode não ser
honrado, e a query viva então reporta `Windowed` -- ver "Headless / Xvfb" abaixo.

### Fail-high

- `set_window_mode(mode)` retorna `false` (no-op, modo atual inalterado) quando o `App` está
  `!ok()`, ou quando `mode` não é um dos quatro enumeradores válidos (ex.: um cast cru de um
  inteiro fora de faixa). Nunca crasha com input ruim.
- `AppConfig::window_mode` setado com um enumerador inválido na construção cai pra `Windowed`,
  documentado, nunca um `create()` que falha.
- `get_window_size` retorna `w = h = 0` quando `!ok()`.

### Notas de plataforma (honestas, não escondidas)

- **Wayland:** `FullscreenExclusive` é **emulado** pelo compositor -- não há troca real de video
  mode (é o comportamento documentado do próprio GLFW no Wayland, não uma limitação do glintfx);
  se comporta como `FullscreenDesktop` com o compositor escalando a superfície. Restaurar a
  posição da janela ao sair de fullscreen/maximizado é **best-effort**: o Wayland não deixa um
  cliente ler nem setar a própria posição de janela, então uma janela que nasceu fullscreen e
  volta pra `Windowed` cai num default documentado em vez de "onde estava".
- **X11 / XWayland:** os quatro modos são literais -- `FullscreenExclusive` faz troca real de
  modo, posição de janela é legível/gravável, restauração é exata.
- **Headless / Xvfb:** não há window manager nem painel sob Xvfb, então "maximizar respeita a work
  area" **não é verificável por máquina** ali -- o pedido de maximizar pode nem ser honrado sem um
  WM, e `window_mode()` pode então reportar `Windowed` pra um pedido `Maximized`. Isso é esperado,
  não um bug; é por isso que a suíte headless desta fatia trata esse único caso como tolerante
  (aceita as duas respostas) e o comportamento real é provado pelo smoke manual abaixo, numa
  sessão de desktop de verdade.
- **Só monitor primário.** Esta fatia sempre mira `glfwGetPrimaryMonitor()`; não há
  `monitor_index` nem API de enumeração de monitores ainda. Ver "Fora de escopo" abaixo.

### Referência de API

| Símbolo | Notas |
| :--- | :--- |
| `WindowMode` (enum class) | `Windowed`, `Maximized`, `FullscreenDesktop`, `FullscreenExclusive`. `glintfx/include/glintfx/window_mode.hpp`. |
| `AppConfig::window_mode` | Modo inicial. Default `Windowed`. Valor inválido -> `Windowed` (documentado, nunca falha o `create()`). |
| `App::set_window_mode(WindowMode mode) -> bool` | Troca em runtime. `false` (no-op) quando `!ok()` ou `mode` inválido; `true` quando o pedido foi emitido pro window system (o WM/compositor tem a palavra final -- consulte `window_mode()` pro resultado vivo). Repedir o modo corrente é um no-op seguro retornando `true`. |
| `App::window_mode() const -> WindowMode` | Vivo, derivado do window system, não um eco do último pedido. `Windowed` quando `!ok()`. |
| `App::get_window_size(int& w, int& h) const -> void` | Tamanho corrente do framebuffer em pixels físicos -- o mesmo tamanho em que `render()`/`snapshot()` compõem. `w = h = 0` quando `!ok()`. |

### Fora de escopo (nesta fatia)

- **Multi-monitor / `monitor_index`.** Só monitor primário; expor um índice sem API de
  enumeração de monitores seria meia-API. Fatia futura se um consumidor real precisar escolher um
  display específico.
- **`resizable`, borderless-windowed (janela sem borda NÃO-fullscreen), tamanho mín/máx de
  janela, callback de mudança de modo, controle de vsync/refresh-rate.** Nenhum tem consumidor
  confirmado ainda; cada um seria superfície pública nova por conta própria.
- **Encaminhamento de roda do mouse no `App`.** Uma lacuna separada e pré-existente (`App` não
  tem NENHUM caminho de wheel; `UiLayer` tem desde a v0.4.0) -- não relacionada a modos de janela,
  não tocada por esta fatia.

### Verificação manual (uma sessão de desktop de verdade)

O Xvfb não tem window manager nem painel, então a única alegação deste doc que realmente importa
-- "maximizar respeita a barra de tarefas" -- não pode ser provada pela suíte automatizada.
Reproduza você mesmo numa sessão KDE/GNOME (ou outro WM) real:

1. Builde a suíte de testes (`-DGLINTFX_BUILD_TESTS=ON`) e rode
   `tests/app_window_mode_transitions_smoke` diretamente, **sem** o wrapper Xvfb, na sua sessão
   real. Ele cicla `Windowed -> Maximized -> Windowed -> FullscreenDesktop -> Windowed ->
   FullscreenExclusive -> Maximized -> Windowed`, então você vê os quatro modos e as transições
   deles numa só corrida -- observe a janela em vez de ler a saída do teste.
2. **`Maximized`:** a janela cresce até preencher a tela mas a barra de tarefas/painel continua
   visível e descoberto. Este é o critério do GusWorld.
3. **`FullscreenDesktop`:** a janela cobre tudo, painel incluso -- by design. Alt-Tab pra fora e
   de volta sem sujeira.
4. **`FullscreenExclusive`:** no X11, uma troca real de modo acontece quando `width`×`height`
   difere da resolução do desktop; no Wayland, se comporta como `FullscreenDesktop` (escalado) --
   confirme que bate com a nota de plataforma acima, não espere uma troca literal de resolução lá.
5. Observe cada transição: nenhum frame preto persistente, e voltar pra `Windowed` a partir de
   fullscreen ou maximizado cai num tamanho/posição plausível (posição exata é best-effort no
   Wayland, conforme a nota acima).
