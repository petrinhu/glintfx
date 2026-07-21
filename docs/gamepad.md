# Gamepad (A2-GAMEPAD reference and how-to)

> **EN:** `glintfx::Gamepads` API reference, permission and degradation behavior, mapping model
> (built-in database + runtime overrides), and the raw evdev surface. Diátaxis type:
> **how-to / reference**. Audience: a developer wiring gamepad input into a glintfx-based game
> or app, on Linux.
> **PT:** Referência da API `glintfx::Gamepads`, contrato de permissões/degradação, modelo de
> mapeamento (banco embutido + overrides em runtime), e a superfície crua evdev. Tipo Diátaxis:
> **how-to / reference**. Audiência: dev conectando input de gamepad num jogo ou app baseado em
> glintfx, no Linux.

`glintfx::Gamepads` (`glintfx/include/glintfx/gamepad.hpp`) is the gamepad-input facade for the
`GLINTFX_MODULE_GAMEPAD` atom ([ADR-0015](adr/0015-framework-2d-atomized-architecture.md) (b)
"gamepad", [ADR-0016](adr/0016-gamepad-atom.md)). It depends on `core` ONLY -- no GL, no window,
no RmlUi type -- so it is testable headless, same independence class as `i18n`/`audio`. It talks
to the kernel directly via raw evdev (`/dev/input/event*` + `ioctl`), **zero new dependency**:
no `libSDL`, no `libudev`, no `libevdev`. It is **Linux-only by nature** (evdev is a Linux kernel
interface) -- the CMake option force-disables itself on any other `CMAKE_SYSTEM_NAME`.

---

## English

### Lifecycle (read this before anything else)

```cpp
glintfx::Gamepads pads;                    // touches nothing yet
glintfx::GamepadsConfig cfg;                // defaults: /dev/input, hotplug ON, builtin DB ON
pads.init(cfg);

// inside your App's per-frame hook (see glintfx::App::set_frame_callback, A1-INPUT):
app.set_frame_callback([&](float /*dt*/) {
  pads.poll();
  if (pads.connected(0) && pads.button(0, glintfx::GamepadButton::South)) {
    // jump, confirm, whatever the game needs
  }
  float x = pads.axis(0, glintfx::GamepadAxis::LeftX);   // [-1, 1]
});

// ... later, on shutdown ...
pads.shutdown();                            // or just let `pads` go out of scope
```

- `Gamepads()` does not touch any device -- construction is always cheap.
- `init(GamepadsConfig)` scans `dev_dir` once and arms hotplug watching. Returns `true` even
  with **zero pads found** (no controller plugged in, or every device inaccessible without
  permission -- see "Permissions" below) -- `false` is reserved for "already initialized, call
  `shutdown()` first".
- `poll()` is a per-frame pump: drains pending events from every connected pad's fd and, if
  `hotplug` is on, the `inotify` watch. Call it once per frame, typically from the same hook
  `App::set_frame_callback`/`UiLayer`'s host loop already gives you (A1-INPUT).
- **Every accessor before a successful `init()`, or after `shutdown()`, returns
  `false`/`0`/`""`.** Never a crash, never a use-after-free -- same fail-high discipline as every
  other atom in this house (`Audio`, `I18n`).
- `shutdown()` is idempotent (closes every fd + the inotify watch); the destructor calls it.
  Re-`init()` after a clean `shutdown()` is fully supported.

### Permissions (read this before filing a "0 pads" bug)

A raw evdev node (`/dev/input/eventN`) is owned by `root:input` with mode `0660` on virtually
every modern distribution's default udev rules. **`init()` never requires root and never
crashes on a permission-denied device** -- a pad your process cannot `open()` is simply skipped
(not counted as connected, not retried in a hot loop), and `connected_count()` reports the true
reachable count. If every pad on the machine reads `0`:

1. Confirm your user is in the `input` group: `groups $USER`. If not:
   `sudo usermod -aG input $USER`, then **log out and back in** (group membership is read at
   login, not live).
2. If the distro's default rule does not tag your specific device, an example udev rule
   granting the `input` group access to joystick-class devices:
   ```
   # /etc/udev/rules.d/70-glintfx-gamepad.rules
   SUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="1", MODE="0660", GROUP="input"
   ```
   `sudo udevadm control --reload-rules && sudo udevadm trigger`.
3. **The hotplug race this module already handles for you:** udev sometimes creates the device
   node microseconds *before* applying its group/mode rule -- a naive one-shot `open()` right
   at `IN_CREATE` can race a still-`root:root`/`0600` node and see `EACCES`, even though the
   permission fix lands a moment later. `Gamepads` also watches `IN_ATTRIB` (an attribute
   change, which is exactly what the group/mode rule application looks like to `inotify`) and
   retries the open then -- this is why hotplugging a controller "just works" without a
   sleep-and-retry loop in your game code.

### Mapping (built-in database + your own)

Every pad's raw button/axis codes get resolved to the normalized `GamepadButton`/`GamepadAxis`
layout via a **mapping database**, in the exact text format the community
[SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) project uses
(`guid,name,bN/aN/hH.M/...,platform:Linux,` -- see
`glintfx/third_party/gamecontrollerdb/README.md`): this is consumed as **DATA** by glintfx's own
clean-room parser, no SDL code involved.

- `GamepadsConfig::use_builtin_db = true` (default): the Linux subset of that community database
  is embedded in the binary (`glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.inc`,
  generated by `tools/gen_gamepad_db.py`) and consulted first.
- `load_mappings_file(path)` / `load_mappings_text(text)`: add more entries at runtime (a
  newer/community mapping file you ship yourself, or a hand-written line for a quirky pad).
  Returns the count of entries actually added; malformed input is skipped line-by-line
  (hostile-safe), never aborts.
- **If no database entry matches a pad** (unknown GUID), it still works: glintfx falls back to
  the **kernel's own gamepad specification**
  (`Documentation/input/gamepad.rst` -- `BTN_SOUTH`/`BTN_EAST`/..., `ABS_X`/`ABS_Y`/`ABS_RX`/
  `ABS_RY`, `ABS_HAT0X`/`ABS_HAT0Y` for the d-pad), which modern kernel drivers (`xpad`,
  `hid-playstation`, `hid-nintendo`, ...) already normalize to. The database exists to correct
  the pads that *don't* follow that spec cleanly, not to be the only path to a working layout.

### The raw surface (why it exists)

The normalized `button()`/`axis()` pair covers the common 15-button/6-axis layout, but the
leader's explicit decision for this slice was **do not restrict input to that standard
layout** -- some real controllers (flight sticks, 12-button fighting sticks, wheels) have more.
`raw_button_count`/`raw_button`/`raw_button_code` and `raw_axis_count`/`raw_axis`/
`raw_axis_value`/`raw_axis_code` expose every button/axis the device itself reports, indexed in
SDL-compatible enumeration order (so a raw index lines up with the `bN`/`aN` numbering the
mapping database format itself uses) -- `raw_axis` is normalized by the device's own reported
`absinfo` range, `raw_axis_value` is the untouched kernel value.

### API reference

| Method | Returns | Notes |
| :--- | :--- | :--- |
| `init(GamepadsConfig cfg = {})` | `bool` | Scans `dev_dir`, arms hotplug. `true` even with 0 pads (permission/no-device). `false` only if already initialized. |
| `shutdown()` | `void` | Idempotent; closes every fd + inotify watch. |
| `is_initialized()` | `bool` | |
| `poll()` | `void` | Per-frame pump: device events + hotplug. No-op before `init()`. |
| `connected_count()` | `int` | |
| `connected(int pad)` | `bool` | `pad` outside `[0, kMaxPads)` -> `false`. |
| `name(int pad)` | `const char*` | `""` if invalid/disconnected. |
| `guid(int pad)` | `const char*` | 32 lowercase hex chars (SDL GUID format), `""` if invalid. |
| `button(int pad, GamepadButton b)` | `bool` | Normalized layout (via mapping DB or kernel-spec default). |
| `axis(int pad, GamepadAxis a)` | `float` | Sticks `[-1, 1]`, triggers `[0, 1]`. `0.0f` if invalid. |
| `raw_button_count(int pad)` | `int` | |
| `raw_button(int pad, int index)` | `bool` | |
| `raw_button_code(int pad, int index)` | `std::uint16_t` | Raw evdev code; `0` if invalid. |
| `raw_axis_count(int pad)` | `int` | |
| `raw_axis(int pad, int index)` | `float` | Normalized by the device's own `absinfo`. |
| `raw_axis_value(int pad, int index)` | `std::int32_t` | Untouched kernel value. |
| `raw_axis_code(int pad, int index)` | `std::uint16_t` | |
| `load_mappings_file(const char* path)` | `int` | Entries added; `0` on failure/hostile input. |
| `load_mappings_text(const char* text)` | `int` | Same; `nullptr` -> `0`. |
| `set_connection_callback(fn(int pad, bool connected))` | `void` | Copy-before-invoke discipline (same as every other glintfx callback, AUD-TEC-3); `nullptr` is a safe no-op. |

`GamepadButton`: `South, East, West, North, DpadUp, DpadDown, DpadLeft, DpadRight, LeftBumper,
RightBumper, LeftStick, RightStick, Start, Back, Guide` (+ `Count`).
`GamepadAxis`: `LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger` (+ `Count`).
`Gamepads::kMaxPads = 8`.

### Limits of this slice (honest, not silent)

- **No rumble/force-feedback** (`EV_FF`) -- a future slice if a real consumer needs it.
- **No deadzone imposed.** Axis values are the raw, absinfo-normalized signal; deadzone is a
  game-design decision, not a framework one. Apply your own threshold on `axis()`/`raw_axis()`.
- **No `App`/`AppConfig` integration in this slice.** Create a `glintfx::Gamepads` yourself and
  call `poll()` inside `App::set_frame_callback` (A1-INPUT) -- zero coupling risk to the
  existing `App`. An `AppConfig`-level convenience (auto-owned pads instance) is a future slice
  if requested.
- **8 pads maximum** (`kMaxPads`), fixed-size slots; a reconnect reuses the lowest free slot.
- Flight sticks and other joystick-class devices are *detected* (the same capability-bit test
  that finds gamepads also finds them), but good mapping-database coverage for that category is
  not a promise of this slice -- only gamepads got test-vector attention here.

### Related

- `glintfx/include/glintfx/gamepad.hpp` -- full doc-commented API contract (source of truth).
- `glintfx/include/glintfx/gamepad_types.hpp` -- `GamepadButton`/`GamepadAxis` enums.
- `glintfx/third_party/gamecontrollerdb/README.md` -- vendored mapping database provenance/pin,
  license, and re-sync recipe.
- `docs/adr/0016-gamepad-atom.md` -- the atom-naming decision (`GLINTFX_MODULE_GAMEPAD`, own atom
  vs. folding into the reserved `GLINTFX_MODULE_INPUT`).
- `docs/adr/0015-framework-2d-atomized-architecture.md` -- the atomized-module architecture this
  module is a slice of.

---

## Português

### Ciclo de vida (leia isto antes de qualquer coisa)

```cpp
glintfx::Gamepads pads;                    // não toca em nada ainda
glintfx::GamepadsConfig cfg;                // defaults: /dev/input, hotplug ON, DB embutida ON
pads.init(cfg);

// dentro do hook por-frame do seu App (ver glintfx::App::set_frame_callback, A1-INPUT):
app.set_frame_callback([&](float /*dt*/) {
  pads.poll();
  if (pads.connected(0) && pads.button(0, glintfx::GamepadButton::South)) {
    // pular, confirmar, o que o jogo precisar
  }
  float x = pads.axis(0, glintfx::GamepadAxis::LeftX);   // [-1, 1]
});

// ... depois, no desligamento ...
pads.shutdown();                            // ou simplesmente deixe `pads` sair de escopo
```

- `Gamepads()` não toca em nenhum device -- a construção é sempre barata.
- `init(GamepadsConfig)` faz um scan único do `dev_dir` e arma o watch de hotplug. Retorna
  `true` mesmo com **zero pads encontrados** (nenhum controle plugado, ou todo device
  inacessível sem permissão -- ver "Permissões" abaixo) -- `false` é reservado pra "já
  inicializado, chame `shutdown()` primeiro".
- `poll()` é o pump por-frame: drena eventos pendentes do fd de cada pad conectado e, se
  `hotplug` estiver ligado, do watch `inotify`. Chame uma vez por frame, tipicamente do mesmo
  hook que `App::set_frame_callback`/o loop do host do `UiLayer` já dão (A1-INPUT).
- **Todo acesso antes de um `init()` bem-sucedido, ou depois de `shutdown()`, retorna
  `false`/`0`/`""`.** Nunca um crash, nunca um use-after-free -- mesma disciplina fail-high de
  todo outro átomo da casa (`Audio`, `I18n`).
- `shutdown()` é idempotente (fecha todo fd + o watch inotify); o destrutor chama. Re-`init()`
  depois de um `shutdown()` limpo é totalmente suportado.

### Permissões (leia isto antes de abrir um bug de "0 pads")

Um nó evdev cru (`/dev/input/eventN`) pertence a `root:input` com modo `0660` em praticamente
toda distro moderna com as regras udev default. **`init()` NUNCA exige root e NUNCA crasha num
device com permissão negada** -- um pad que o processo não consegue abrir é simplesmente pulado
(não conta como conectado, não é retentado num loop apertado), e `connected_count()` reporta a
contagem real alcançável. Se todo pad da máquina lê `0`:

1. Confirme que seu usuário está no grupo `input`: `groups $USER`. Se não estiver:
   `sudo usermod -aG input $USER`, depois **saia e entre de novo na sessão** (a associação de
   grupo é lida no login, não ao vivo).
2. Se a regra default da distro não marcar seu device específico, um exemplo de regra udev
   concedendo acesso do grupo `input` a devices classe-joystick:
   ```
   # /etc/udev/rules.d/70-glintfx-gamepad.rules
   SUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="1", MODE="0660", GROUP="input"
   ```
   `sudo udevadm control --reload-rules && sudo udevadm trigger`.
3. **A corrida de hotplug que este módulo já resolve pra você:** o udev às vezes cria o nó do
   device microssegundos *antes* de aplicar a regra de grupo/modo -- um `open()` ingênuo de
   tentativa única bem no `IN_CREATE` pode competir com um nó ainda `root:root`/`0600` e ver
   `EACCES`, mesmo que o fix de permissão chegue um instante depois. `Gamepads` também observa
   `IN_ATTRIB` (mudança de atributo, exatamente a cara que a aplicação da regra de grupo/modo
   tem pro `inotify`) e retenta o open então -- é por isso que fazer hotplug de um controle
   "simplesmente funciona" sem um loop de sleep-e-retry no código do seu jogo.

### Mapeamento (banco embutido + o seu próprio)

Os códigos crus de botão/eixo de cada pad são resolvidos pro layout normalizado
`GamepadButton`/`GamepadAxis` via um **banco de mapeamento**, no formato de texto exato que o
projeto da comunidade [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) usa
(`guid,name,bN/aN/hH.M/...,platform:Linux,` -- ver
`glintfx/third_party/gamecontrollerdb/README.md`): consumido como **DADO** pelo parser
clean-room próprio do glintfx, nenhum código SDL envolvido.

- `GamepadsConfig::use_builtin_db = true` (default): o subconjunto Linux desse banco da
  comunidade é embutido no binário
  (`glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.inc`, gerado por
  `tools/gen_gamepad_db.py`) e consultado primeiro.
- `load_mappings_file(path)` / `load_mappings_text(text)`: adiciona mais entradas em runtime (um
  arquivo de mapeamento mais novo/da comunidade que você distribui, ou uma linha escrita à mão
  pra um pad excêntrico). Retorna a contagem de entradas de fato adicionadas; entrada malformada
  é pulada linha-a-linha (hostile-safe), nunca aborta.
- **Se nenhuma entrada do banco casar com um pad** (GUID desconhecido), ele ainda funciona:
  o glintfx cai pra **especificação própria de gamepad do kernel**
  (`Documentation/input/gamepad.rst` -- `BTN_SOUTH`/`BTN_EAST`/..., `ABS_X`/`ABS_Y`/`ABS_RX`/
  `ABS_RY`, `ABS_HAT0X`/`ABS_HAT0Y` pro d-pad), à qual drivers de kernel modernos (`xpad`,
  `hid-playstation`, `hid-nintendo`, ...) já normalizam. O banco existe pra corrigir os pads que
  *não* seguem essa spec de forma limpa, não pra ser o único caminho pra um layout funcional.

### A superfície crua (por que existe)

O par normalizado `button()`/`axis()` cobre o layout comum de 15 botões/6 eixos, mas a decisão
explícita do líder pra esta fatia foi **não restringir o input a esse layout padrão** -- alguns
controles reais (manches de voo, fighting sticks de 12 botões, volantes) têm mais. Os pares
`raw_button_count`/`raw_button`/`raw_button_code` e
`raw_axis_count`/`raw_axis`/`raw_axis_value`/`raw_axis_code` expõem todo botão/eixo que o próprio
device reporta, indexados na ordem de enumeração compatível-SDL (então um índice cru bate com a
numeração `bN`/`aN` que o próprio formato do banco de mapeamento usa) -- `raw_axis` é normalizado
pelo `absinfo` que o próprio device reporta, `raw_axis_value` é o valor de kernel intocado.

### Referência de API

| Método | Retorna | Notas |
| :--- | :--- | :--- |
| `init(GamepadsConfig cfg = {})` | `bool` | Faz scan do `dev_dir`, arma hotplug. `true` mesmo com 0 pads (permissão/sem device). `false` só se já inicializado. |
| `shutdown()` | `void` | Idempotente; fecha todo fd + o watch inotify. |
| `is_initialized()` | `bool` | |
| `poll()` | `void` | Pump por-frame: eventos de device + hotplug. No-op antes de `init()`. |
| `connected_count()` | `int` | |
| `connected(int pad)` | `bool` | `pad` fora de `[0, kMaxPads)` -> `false`. |
| `name(int pad)` | `const char*` | `""` se inválido/desconectado. |
| `guid(int pad)` | `const char*` | 32 chars hex minúsculo (formato GUID SDL), `""` se inválido. |
| `button(int pad, GamepadButton b)` | `bool` | Layout normalizado (via banco de mapeamento ou default kernel-spec). |
| `axis(int pad, GamepadAxis a)` | `float` | Sticks `[-1, 1]`, gatilhos `[0, 1]`. `0.0f` se inválido. |
| `raw_button_count(int pad)` | `int` | |
| `raw_button(int pad, int index)` | `bool` | |
| `raw_button_code(int pad, int index)` | `std::uint16_t` | Código evdev cru; `0` se inválido. |
| `raw_axis_count(int pad)` | `int` | |
| `raw_axis(int pad, int index)` | `float` | Normalizado pelo `absinfo` do próprio device. |
| `raw_axis_value(int pad, int index)` | `std::int32_t` | Valor de kernel intocado. |
| `raw_axis_code(int pad, int index)` | `std::uint16_t` | |
| `load_mappings_file(const char* path)` | `int` | Entradas adicionadas; `0` em falha/entrada hostil. |
| `load_mappings_text(const char* text)` | `int` | Idem; `nullptr` -> `0`. |
| `set_connection_callback(fn(int pad, bool connected))` | `void` | Disciplina copy-before-invoke (igual todo outro callback do glintfx, AUD-TEC-3); `nullptr` é no-op seguro. |

`GamepadButton`: `South, East, West, North, DpadUp, DpadDown, DpadLeft, DpadRight, LeftBumper,
RightBumper, LeftStick, RightStick, Start, Back, Guide` (+ `Count`).
`GamepadAxis`: `LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger` (+ `Count`).
`Gamepads::kMaxPads = 8`.

### Limites desta fatia (honestos, não silenciosos)

- **Sem rumble/força tátil** (`EV_FF`) -- fatia futura se um consumidor real precisar.
- **Sem deadzone imposta.** Os valores de eixo são o sinal cru, normalizado por absinfo;
  deadzone é decisão de design de jogo, não do framework. Aplique seu próprio limiar em cima de
  `axis()`/`raw_axis()`.
- **Sem integração `App`/`AppConfig` nesta fatia.** Crie um `glintfx::Gamepads` você mesmo e
  chame `poll()` dentro de `App::set_frame_callback` (A1-INPUT) -- zero risco de acoplamento no
  `App` existente. Uma conveniência nível-`AppConfig` (instância de pads auto-possuída) é fatia
  futura se pedida.
- **8 pads no máximo** (`kMaxPads`), slots de tamanho fixo; uma reconexão reusa o menor slot
  livre.
- Manches de voo e outros devices classe-joystick são *detectados* (o mesmo teste de bit de
  capacidade que acha gamepads também acha eles), mas boa cobertura de banco de mapeamento pra
  essa categoria não é promessa desta fatia -- só gamepads receberam atenção de vetor de teste
  aqui.

### Relacionados

- `glintfx/include/glintfx/gamepad.hpp` -- contrato de API completo com doc-comments (fonte da
  verdade).
- `glintfx/include/glintfx/gamepad_types.hpp` -- enums `GamepadButton`/`GamepadAxis`.
- `glintfx/third_party/gamecontrollerdb/README.md` -- procedência/pin do banco de mapeamento
  vendorizado, licença, e receita de re-sincronização.
- `docs/adr/0016-gamepad-atom.md` -- a decisão de nomeação do átomo (`GLINTFX_MODULE_GAMEPAD`,
  átomo próprio vs. dobrar no `GLINTFX_MODULE_INPUT` reservado).
- `docs/adr/0015-framework-2d-atomized-architecture.md` -- a arquitetura de módulos atomizados da
  qual este módulo é uma fatia.
