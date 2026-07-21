# Plan A2-GAMEPAD -- evdev gamepad atom / Plano A2-GAMEPAD -- átomo gamepad via evdev

- **Date / Data:** 2026-07-21
- **Author / Autor:** Caetano (CTO), for the orchestrator to execute with up to 4 sonnet agents
- **Governs / Governa:** framework-2D phase A2 (ADR-0015 (g)); target release v0.15.0
- **Leader decisions already made (DO NOT reopen) / Decisões do líder já tomadas (NÃO reabrir):**
  raw evdev, ZERO new dependency (no libevdev/libudev/SDL); Linux-only, honest platform gate;
  SDL `gamecontrollerdb.txt` used as DATA only (vendored text, never linked SDL code);
  expose ALL raw buttons/axes, not just the standard layout.

---

## 1. Summary (EN, concise)

A2 delivers `GLINTFX_MODULE_GAMEPAD`: a new opt-in atom in the same independence class as
`i18n`/`audio` -- depends on `core` only, zero GL/window/RmlUi/third-party type in the public
header, OFF removes code+symbols (`nm` proof), tests are pure (no Xvfb). It is **Linux-only by
nature** (evdev is a kernel interface): the CMake gate forces the module OFF on any
non-Linux `CMAKE_SYSTEM_NAME` with a `STATUS` message (graceful, not FATAL).

**Naming decision:** ADR-0015 (b) lists gamepad as the window-independent backend of the
reserved `input` module. We ship it as its **own atom** `GLINTFX_MODULE_GAMEPAD` (mirror of
audio/i18n; ADR-0015's own consequences name "gamepad-only" as a valuable composition).
`GLINTFX_MODULE_INPUT` stays reserved. A short **ADR-0016** records this refinement (the
module list only changes by ADR).

**Architecture (3 layers, 2 of them 100% headless-testable):**

1. **Decode core** (`src/gamepad_decode.{hpp,cpp}`) -- pure state machine: byte-stream
   reassembly of `struct input_event` (24 bytes on x86-64), `EV_KEY`/`EV_ABS`/`EV_SYN`
   handling, `SYN_DROPPED` flag, absinfo-based normalization, hostile-input hard guards.
   No fd, no ioctl -- fed synthetic buffers in tests (dependency injection by construction).
2. **Mapping** (`src/gamepad_mapping.{hpp,cpp}`) -- SDL-GUID construction from
   `EVIOCGID` fields, SDL-compatible joystick index derivation from capability bitmaps
   (clean-room, from documented behavior), `gamecontrollerdb.txt` text parser (hostile-safe),
   default mapping per the kernel gamepad spec (`Documentation/input/gamepad.rst`) for
   unmatched pads. Pure, headless-testable.
3. **Device layer + public API** (`include/glintfx/gamepad.hpp` + `gamepad_types.hpp`,
   `src/gamepad.cpp`) -- scan of `<dev_dir>/event*`, capability-based gamepad detection
   (never grabs keyboard/mouse), `O_RDONLY|O_NONBLOCK` open with **graceful EACCES
   degradation** (no root, no crash, documented `input`-group/udev requirement), hotplug via
   raw inotify (`IN_CREATE|IN_ATTRIB|IN_DELETE` -- `IN_ATTRIB` catches the udev
   permission-race), poll-based pump fitting the A1 `set_frame_callback` frame loop.
   Testable headless via the `dev_dir` config override (empty dir, garbage files,
   permission-denied files).

**Execution:** 4 agents. Wave 1: Agent A (decode+mapping+CMake) parallel with Agent C
(vendored DB + docs + ADR-0016 + CI conformance). Wave 2: Agent B (device layer + public
API; serialized -- shares CMakeLists/tests-CMakeLists with A). Wave 3: Agent D (adversarial
review that EXECUTES). One heavy glintfx build at a time (memory pressure); heavy build dirs
under `/var/tmp` with `TMPDIR=/var/tmp`.

**CI:** `lint-and-scan`'s recursive globs (`src/**/*.cpp`, cppcheck via
`compile_commands.json`) cover the new TUs automatically on the Linux runners.
`windows-atoms.yml` is **NOT touched**: the gamepad atom is Linux-only, stays out of the
`tests/win-atoms/` harness and out of that workflow's path filters; a Windows build of the
other atoms ignores gamepad cleanly because of the CMake platform gate. **No Windows gamepad
test is fabricated.** No new preset (`gamepad-only` would require `GLINTFX_MODULE_UI=OFF`,
which does not exist yet -- same documented deferral as A3's `audio-only`).

---

## 2. Resumo e decisões (PT)

O A2 entrega o átomo opt-in `GLINTFX_MODULE_GAMEPAD`, espelho de `audio`/`i18n`: depende SÓ
de `core`, nenhum tipo de terceiro no header público (`<linux/input.h>` vive só em `src/`),
OFF remove código+símbolos (prova `nm`), testes puros sem GL/Xvfb. **Linux-only honesto**:
gate de plataforma no CMake (força OFF fora de Linux com `message(STATUS ...)`, sem FATAL).

### 2.1 Decisões de engenharia (tomadas por mim; menores, seguir; marcadas ⚑ = confirmar retroativamente com o líder)

| # | Decisão | Racional |
|---|---|---|
| D1 | **Átomo próprio `GLINTFX_MODULE_GAMEPAD`** (não dobrar num `GLINTFX_MODULE_INPUT`) | teclado/mouse está fundido no átomo window (A1); um INPUT que só contivesse gamepad seria nome mentiroso. ADR-0015 já cita a composição "gamepad-only". `GLINTFX_MODULE_INPUT` segue reservado. **⚑ ADR-0016 curto registra o refinamento** (lista de módulos só muda por ADR). |
| D2 | **Hotplug via inotify cru** (não netlink) | inotify em `/dev/input` é 1 fd + 2 syscalls, sem parsing de mensagem kernel; netlink uevent exige parse de socket raw e duplica o que o inotify já dá aqui. `IN_ATTRIB` resolve a corrida de permissão do udev (device criado antes do chmod do grupo). |
| D3 | **Hotplug JÁ nesta fatia** (não adiar) | custo marginal baixo (um watch + re-scan incremental) e é o valor citado pelo ADR ("hotplug sem SDL"); adiar criaria API pública que muda depois. |
| D4 | **8 pads máx** (`kMaxPads = 8`, array fixo de slots) | cobre multiplayer local real; slots estáveis enquanto conectado; reconexão reusa o menor slot livre. |
| D5 | **Mapeamento default = spec de gamepad do kernel** (`BTN_SOUTH`..., `ABS_X/Y/RX/RY`, gatilhos `ABS_BRAKE/GAS` se presentes senão `ABS_Z/RZ`, dpad `ABS_HAT0X/Y`) | drivers modernos (xpad, hid-playstation, hid-nintendo) já normalizam pra essa spec; a DB só corrige os fora-da-curva. |
| D6 ⚑ | **Vendorizar o subconjunto Linux do `gamecontrollerdb.txt`** (zlib, comunidade) como TEXTO commitado + header gerado (`.inc`) embutido no módulo (`use_builtin_db=true` default; ~90-150 KB de `.rodata`, some com o módulo OFF) | "batteries-included": consumidor não precisa carregar arquivo. Ferramenta `tools/gen_gamepad_db.py` no padrão offline/reprodutível do `gen_glloader.py`. API `load_mappings_file/text` permite atualizar/estender em runtime. Atribuição no `NOTICE`. |
| D7 | **Detecção de gamepad por bits de capacidade**: aceitar device com `EV_ABS` presente E algum bit `EV_KEY` na faixa `[BTN_JOYSTICK=0x120, BTN_DIGI=0x140)` | pega gamepads (`BTN_SOUTH`=0x130) e joysticks (`BTN_TRIGGER`=0x120) sem sequestrar teclado (códigos <0x100) nem mouse (`BTN_MOUSE`=0x110-0x117). |
| D8 | **`SYN_DROPPED` tratado de verdade**: decoder marca a flag; camada de device ressincroniza via `EVIOCGKEY`/`EVIOCGABS` | ignorar `SYN_DROPPED` = estado fantasma (botão preso). É a classe de bug silencioso que só aparece em produção. |
| D9 | **Sem deadzone imposta** (normalização crua por absinfo; `flat`/`fuzz` expostos ao consumidor via doc) | deadzone é política do jogo; impor no framework quebraria consumidores que querem o valor cru. Documentado em `docs/gamepad.md`. |
| D10 | **Sem acoplamento no `App` nesta fatia** | consumidor cria `glintfx::Gamepads` e chama `poll()` dentro do `set_frame_callback` do A1. Zero risco de regressão no App; integração `AppConfig`-nível é fatia futura se pedida. |
| D11 | **Sem preset novo** | `gamepad-only` exigiria `GLINTFX_MODULE_UI=OFF` (não existe) -- mesmo adiamento documentado do `audio-only` no A3. |

### 2.2 Contrato da API pública

`include/glintfx/gamepad_types.hpp` (Agente A -- puro, sem include de terceiro):

```cpp
enum class GamepadButton : std::uint8_t { South, East, West, North,
  DpadUp, DpadDown, DpadLeft, DpadRight, LeftBumper, RightBumper,
  LeftStick, RightStick, Start, Back, Guide, Count };
enum class GamepadAxis : std::uint8_t { LeftX, LeftY, RightX, RightY,
  LeftTrigger, RightTrigger, Count };
```

`include/glintfx/gamepad.hpp` (Agente B -- pimpl, espelho do contrato RAII do `Audio`):

```cpp
struct GamepadsConfig {
  const char* dev_dir = "/dev/input";  // override de teste/headless
  bool hotplug        = true;          // inotify no dev_dir
  bool use_builtin_db = true;          // DB vendorizada embutida
};
class Gamepads {                        // rule-of-5 deletado, pimpl, como Audio
public:
  static constexpr int kMaxPads = 8;
  Gamepads(); ~Gamepads();             // ctor não toca em nada; dtor chama shutdown()
  bool init(const GamepadsConfig& cfg = {});  // true mesmo com 0 pads (EACCES = degrada);
                                              // false só se já inicializado
  void shutdown();                     // idempotente, determinístico
  bool is_initialized() const;
  void poll();                         // pump por frame: eventos + inotify; no-op sem init
  int  connected_count() const;
  bool connected(int pad) const;       // pad em [0, kMaxPads)
  const char* name(int pad) const;     // "" quando slot inválido/desconectado
  const char* guid(int pad) const;     // 32 hex chars (formato SDL), "" inválido
  // --- layout normalizado (via DB ou default kernel-spec) ---
  bool  button(int pad, GamepadButton b) const;
  float axis(int pad, GamepadAxis a) const;   // sticks [-1,1], triggers [0,1]; 0.0f inválido
  // --- superfície CRUA completa (decisão do líder: não restringir) ---
  int           raw_button_count(int pad) const;
  bool          raw_button(int pad, int index) const;
  std::uint16_t raw_button_code(int pad, int index) const;  // código evdev; 0 inválido
  int           raw_axis_count(int pad) const;
  float         raw_axis(int pad, int index) const;         // normalizado por absinfo
  std::int32_t  raw_axis_value(int pad, int index) const;   // valor evdev cru
  std::uint16_t raw_axis_code(int pad, int index) const;
  // --- mapeamentos ---
  int load_mappings_file(const char* path);   // entradas adicionadas; 0 em falha/hostil
  int load_mappings_text(const char* text);   // idem; nullptr => 0
  // --- eventos de conexão (opcional) ---
  void set_connection_callback(std::function<void(int pad, bool connected)> cb);
};
```

Convenções fail-high da casa: todo acesso com `pad`/`index` fora de faixa ou instância
não-inicializada retorna `false`/`0`/`""` -- nunca UB, nunca crash. Callback com a mesma
disciplina copy-before-invoke (AUD-TEC-3) dos callbacks existentes.

### 2.3 Detalhe técnico do decode (contrato do Agente A)

- `struct input_event` x86-64: `{ timeval time(16B); __u16 type; __u16 code; __s32 value; }`
  = 24 bytes. O decoder tem buffer interno de reassemblagem: `feed_bytes(data, len)` aceita
  QUALQUER tamanho de chunk (read parcial de 7 bytes é legal em teoria e é testado).
- Tipos tratados: `EV_KEY` (value 0/1/2 -- 2=autorepeat ignorado p/ botões), `EV_ABS`,
  `EV_SYN` (`SYN_REPORT` no-op nesta fatia -- estado aplicado imediatamente, documentado;
  `SYN_DROPPED` seta flag pegajosa `sync_lost()` + `clear_sync_lost()`). Qualquer outro
  `type` é ignorado.
- Validação dura de faixa: `EV_KEY` com `code >= KEY_MAX(0x2ff)` ignorado; `EV_ABS` com
  `code >= ABS_MAX(0x3f)` ignorado. Nunca indexa array com código não validado.
- Normalização por `EvAbsInfo{value,min,max,fuzz,flat,resolution}` (espelho próprio do
  `input_absinfo`): guard `min >= max` => eixo devolve 0.0f (nunca divisão por zero);
  stick: `2*(v-min)/(max-min) - 1` com clamp [-1,1]; gatilho: `(v-min)/(max-min)` clamp [0,1].
- Hat: `ABS_HAT0X/Y` com value -1/0/+1 => 4 dpad booleans (values fora de {-1,0,1} clampados).

### 2.4 Detalhe técnico do mapeamento (contrato do Agente A)

- **GUID SDL** (16 bytes => 32 hex minúsculo) a partir do `input_id{bustype,vendor,product,version}`
  + nome: bytes 0-1 = bustype LE; 2-3 = CRC16 do nome LE (polinômio 0xA001, init 0 -- variante
  SDL>=2.26); 4-5 = vendor LE; 6-7 = 0; 8-9 = product LE; 10-11 = 0; 12-13 = version LE;
  14-15 = 0. **Match em 2 tentativas**: exato (com CRC) e com bytes 2-3 zerados (a maioria
  das entradas da DB comunitária tem zeros ali).
- **Derivação de índice de joystick compatível-SDL** (clean-room, do comportamento
  documentado): botões enumerados em ordem de código primeiro na faixa
  `[BTN_JOYSTICK, KEY_MAX)`, depois `[0, BTN_JOYSTICK)`; eixos em ordem de código
  `0..ABS_MAX` EXCLUINDO `ABS_HAT0X..ABS_HAT3Y` (0x10-0x17); hats = pares `ABS_HATnX/Y`.
  Entradas `bN`/`aN`/`hH.M` da DB referenciam ESSES índices, não códigos evdev.
- **Parser da DB**: linha = `guid,name,campo:valor,...,platform:Linux`. Aceita `bN`, `aN`,
  `hH.M`, `+aN`/`-aN` (meio-eixo) e sufixo `~` (invertido). Campo desconhecido: ignora e
  segue (fail-high). Linha malformada/truncada/gigante: pula, nunca crasha. Comentários `#`
  e linhas vazias ignorados. Só entradas `platform:Linux` são aceitas.
- **Default kernel-spec** para pad sem match (D5), por código evdev direto.

### 2.5 Detalhe técnico da camada de device (contrato do Agente B)

- Scan inicial: `opendir(cfg.dev_dir)`, entradas `event*`; para cada uma:
  `open(O_RDONLY|O_NONBLOCK|O_CLOEXEC)`; em `EACCES`/`EPERM`: pula (conta como inacessível,
  sem log ruidoso por frame), **sem root, sem crash**. `ioctl EVIOCGBIT(0)` p/ ev bits;
  aplica D7; se não é gamepad: `close`, segue. Se é: `EVIOCGID`, `EVIOCGNAME`,
  `EVIOCGBIT(EV_KEY)`, `EVIOCGBIT(EV_ABS)`, `EVIOCGABS` por eixo presente; monta
  `JoystickLayout`, resolve mapping (DB embutida + carregadas), aloca slot.
- Arquivo regular/lixo chamado `eventN` (cenário de teste): `EVIOCGBIT` falha
  (`ENOTTY`) => pula gracioso.
- `poll()`: para cada pad, `read()` em loop até `EAGAIN`; `ENODEV`/`EBADF` => desconecta
  (fecha fd, libera slot, callback `connected=false`). Bytes lidos vão pro
  `GamepadDecoder::feed_bytes` do pad. Se `sync_lost()`: ressincroniza com
  `EVIOCGKEY`/`EVIOCGABS` e limpa a flag (D8). Depois processa o fd do inotify
  (`IN_CREATE|IN_ATTRIB` => tenta abrir/re-abrir o node; `IN_DELETE` => marca).
- inotify: `inotify_init1(IN_NONBLOCK|IN_CLOEXEC)` + `inotify_add_watch(dev_dir, ...)`.
  Falha do inotify NÃO falha o `init()` (hotplug degrada, scan inicial vale) -- documentado.
- `shutdown()`: fecha todos os fds + inotify; idempotente; dtor chama.

### 2.6 CMake (quem escreve o quê)

Agente A (wave 1) em `glintfx/CMakeLists.txt`:

```cmake
# --- GLINTFX_MODULE_GAMEPAD (real ON/OFF, framework-2D slice A2; Linux-only) ---
option(GLINTFX_MODULE_GAMEPAD "Gamepad module: raw-evdev pads + SDL-db mapping (ADR-0015/0016; core ONLY; Linux-only)" ON)
if(GLINTFX_MODULE_GAMEPAD AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(STATUS "glintfx: gamepad module is Linux-only (raw evdev) -- forcing GLINTFX_MODULE_GAMEPAD=OFF on ${CMAKE_SYSTEM_NAME}")
  set(GLINTFX_MODULE_GAMEPAD OFF)   # shadow local: vale para todos os guards abaixo
endif()
...
if(GLINTFX_MODULE_GAMEPAD)
  add_library(glintfx_gamepad OBJECT src/gamepad_decode.cpp src/gamepad_mapping.cpp)  # B acrescenta src/gamepad.cpp
  target_include_directories(glintfx_gamepad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gamecontrollerdb)
  target_compile_options(glintfx_gamepad PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_gamepad)
  add_library(glintfx::gamepad ALIAS glintfx_gamepad)
endif()
...
if(GLINTFX_MODULE_GAMEPAD)
  target_link_libraries(glintfx PRIVATE glintfx_gamepad)
endif()
```

Zero dep de link nova (evdev/inotify são headers do kernel + syscalls). Agente B (wave 2)
acrescenta `src/gamepad.cpp` à lista do OBJECT e adiciona `#cmakedefine01 GLINTFX_MODULE_GAMEPAD`
ao `config.hpp.in` (com o guard `#error` no `gamepad.hpp`, espelho do `app.hpp`).

Testes (`tests/CMakeLists.txt`, bloco `if(GLINTFX_MODULE_GAMEPAD)` FORA do bloco GLFW e fora
do wrapper Xvfb, espelho do bloco i18n/audio): linkam `glintfx` e incluem os headers internos
via `target_include_directories(<t> PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)`.

### 2.7 Ambiente de execução (TODOS os agentes)

- Build pesado: dir em `/var/tmp` (ex.: `/var/tmp/gfx-a2-<agente>`), `export TMPDIR=/var/tmp`.
  **UM build glintfx por vez na máquina** (swap já pressionado). NUNCA 2 simultâneos.
- Suíte completa (só quando exigida):
  `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest --test-dir <bld> --output-on-failure -j1`
  (os testes de gamepad em si NÃO precisam de Xvfb: `ctest -R gamepad` roda seco).
- Análise estática LOCAL obrigatória por fatia (lição v0.13/v0.14):
  `clang-tidy -p <bld> glintfx/src/gamepad*.cpp` (gate = `clang-analyzer-*`, ver `.clang-tidy`)
  e `cppcheck --project=<bld>/compile_commands.json --error-exitcode=1` restrito aos arquivos
  novos com `--file-filter='*src/gamepad*'` (espelhar a invocação exata do job
  `lint-and-scan` em `.github/workflows/ci.yml`).
- ASan/UBSan da fatia: standalone e barato (sem RmlUi):
  `clang++ -std=c++23 -fsanitize=address,undefined -fno-sanitize-recover=all -g -Iglintfx/include -Iglintfx/src -Iglintfx/third_party/gamecontrollerdb glintfx/src/gamepad_decode.cpp glintfx/src/gamepad_mapping.cpp glintfx/tests/<teste>.cpp -o /var/tmp/t && /var/tmp/t`
  Reprodução no container canônico do CI local (`glintfx-ci:f42`, já buildado):
  `podman run --rm -v "$PWD":/w -w /w glintfx-ci:f42 bash -lc '<mesmo comando clang++ acima>'`
  (o ASan da suíte COMPLETA fica com o gate `claudio`/nightly no push -- não rodar 2 builds pesados locais).
- Commits: Conventional Commits pt-br, citar `A2-GAMEPAD` no corpo, tocar `TODO.md` status no
  mesmo commit (`🔍 Pendente verificação`, nunca `✅` direto). **NÃO fazer push** (só o
  orquestrador, na cadência de onda do modo autônomo).

---

## 3. Waves and file ownership / Ondas e posse de arquivos (disjunção)

| Onda | Agente | Arquivos (posse EXCLUSIVA na onda) |
|---|---|---|
| 1 | **A** `backend-engineer` | `src/gamepad_decode.{hpp,cpp}`, `src/gamepad_mapping.{hpp,cpp}`, `include/glintfx/gamepad_types.hpp`, `tests/gamepad_decode_sanity.cpp`, `tests/gamepad_decode_hostile.cpp`, `tests/gamepad_mapping_sanity.cpp`, `tests/gamepad_mapping_hostile.cpp`, **+ edits**: `glintfx/CMakeLists.txt`, `glintfx/tests/CMakeLists.txt` |
| 1 (paralelo) | **C** `devops-sre` | `tools/gen_gamepad_db.py`, `third_party/gamecontrollerdb/{gamecontrollerdb_linux.txt,gamecontrollerdb_linux.inc,README.md,LICENSE}`, `docs/gamepad.md`, `docs/adr/0016-gamepad-atom.md` (em `docs/adr/` da RAIZ do repo), `NOTICE`, `CHANGELOG.md`, `TODO.md` |
| 2 | **B** `backend-engineer` (2º, ≠A) | `include/glintfx/gamepad.hpp`, `src/gamepad.cpp`, `include/glintfx/config.hpp.in`, `tests/gamepad_device_sanity.cpp`, `tests/gamepad_hotplug_smoke.cpp`, `tests/gamepad_api_hardening.cpp`, **+ edits**: `glintfx/CMakeLists.txt` (append source), `glintfx/tests/CMakeLists.txt` (append tests) |
| 3 | **D** `qa-engineer` (review adversarial) | read-only + execução; achados corrigidos por A/B via orquestrador |

Serialização: B SÓ começa depois de A entregue e verificado (colisão nos 2 CMakeLists +
precisa dos headers de A). C é paralelo a A (zero arquivo em comum); B consome o `.inc` de C.
D por último. Colisão residual A↔C: nenhuma (C não toca CMake nem `src/`).

---

## 4. CI plan / Plano de CI

- **Linux (github, gate primário):** nada a mudar em `ci.yml` -- o glob recursivo do
  clang-tidy (`src/**/*.cpp`) e o cppcheck via `compile_commands.json` pegam as TUs novas;
  a matriz GLFW=ON/OFF builda o módulo (default ON em Linux) e roda os testes puros.
- **`claudio` (heavy.yml, fedora:42):** sem mudança; ASan da suíte cobre o gamepad no PR/tag.
- **Windows (`windows-atoms.yml`): INTOCADO.** O gamepad é Linux-only: fica FORA do harness
  `tests/win-atoms/` e fora dos path-filters do workflow. Prova de não-quebra (Agente C
  verifica): (a) nenhum path novo casa os filtros => o job nem dispara pela mudança;
  (b) o harness compila lista explícita de fontes => não vê `gamepad*.cpp`; (c) num build
  Windows hipotético da árvore principal, o gate `CMAKE_SYSTEM_NAME` desliga o módulo
  limpo. **Não fabricar teste Windows de gamepad.**
- **Prova `nm` do OFF** (Agente D executa): build com `-DGLINTFX_MODULE_GAMEPAD=OFF` =>
  `nm <bld>/libglintfx.a | grep -ci gamepad` == 0.

---

## 5. Briefs (ready to paste / prontos pra colar)

### 5.1 Agente A -- `backend-engineer` -- decode core + mapping (onda 1)

> Você é o backend-engineer da fatia A2-GAMEPAD do glintfx (framework 2D, ADR-0015).
> Leia antes: `glintfx/docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md`
> (este plano -- seções 2.3, 2.4, 2.6, 2.7 são o seu contrato), `docs/adr/0015-...md`,
> `AGENTS.md` (gotchas + convenções), o módulo `i18n`/`audio` como MODELO
> (`glintfx/src/{i18n,audio}.cpp`, blocos CMake correspondentes).
>
> **Entregue (posse exclusiva):**
> 1. `glintfx/include/glintfx/gamepad_types.hpp` -- enums `GamepadButton`/`GamepadAxis`
>    (seção 2.2 do plano, literal). Header público: SÓ `<cstdint>`, zero tipo de terceiro,
>    SPDX + doc-comment bilíngue (en primeiro), padrão da casa.
> 2. `glintfx/src/gamepad_decode.{hpp,cpp}` -- decoder puro (seção 2.3): classe
>    `GamepadDecoder` com `configure_abs(code, EvAbsInfo)`, `feed_bytes(const void*, size_t)`
>    (reassemblagem de `input_event` de 24 bytes a partir de chunks arbitrários),
>    queries de estado (`key_state(code)`, `abs_value(code)`, `abs_normalized(code)`,
>    dpad via hat), `sync_lost()`/`clear_sync_lost()` (SYN_DROPPED). Pode incluir
>    `<linux/input.h>` AQUI (src/, nunca em include/). Validação dura de faixa antes de
>    qualquer indexação; `min>=max` => 0.0f; clamps documentados.
> 3. `glintfx/src/gamepad_mapping.{hpp,cpp}` -- (seção 2.4): `build_guid(...)` (formato SDL,
>    CRC16 0xA001 do nome, match exato E crc-zerado), `derive_layout(keybits, absbits)`
>    (ordem de enumeração compatível-SDL: botões `[0x120,KEY_MAX)` depois `[0,0x120)`;
>    eixos 0..ABS_MAX excluindo hats 0x10-0x17), `MappingDb::parse_text` (parser
>    hostile-safe do gamecontrollerdb: `bN`/`aN`/`hH.M`/`+aN`/`-aN`/`~`, só
>    `platform:Linux`, linha malformada = skip), resolução mapping+layout+decoder =>
>    estado normalizado, e o mapeamento default kernel-spec (D5 do plano).
> 4. CMake (seção 2.6, literal): option + gate Linux + OBJECT lib `glintfx_gamepad`
>    (SÓ os seus 2 .cpp por ora; o agente B acrescenta `gamepad.cpp` depois) + alias +
>    link no agregado, posicionado como irmão dos blocos `glintfx_i18n`/`glintfx_audio`.
> 5. Testes em `glintfx/tests/` + registro em `tests/CMakeLists.txt` (bloco
>    `if(GLINTFX_MODULE_GAMEPAD)` FORA do bloco GLFW, sem Xvfb, espelho do bloco audio;
>    targets linkam `glintfx` + `target_include_directories(<t> PRIVATE
>    ${CMAKE_CURRENT_SOURCE_DIR}/../src)`):
>    - `gamepad_decode_sanity.cpp`: sequência sintética de `input_event` (botão press/release,
>      eixo com absinfo real de um stick -32768..32767, gatilho 0..255, hat -1/0/1),
>      alimentada em chunks de tamanhos variados INCLUINDO 1 byte por vez e 7+17 bytes;
>      autorepeat (value=2) não muda estado de botão; SYN_DROPPED seta flag.
>    - `gamepad_decode_hostile.cpp`: bytes de lixo aleatório (seed fixa), evento truncado no
>      fim do buffer, `type`/`code` fora de faixa (0xFFFF), `value` INT32_MIN/MAX,
>      absinfo com `min==max` e `min>max` -- NUNCA crash, NUNCA leitura fora de faixa,
>      eixo degenerado retorna 0.0f.
>    - `gamepad_mapping_sanity.cpp`: GUID de um `input_id` conhecido (ex.: Xbox 360 USB
>      bus=0x03 vendor=0x045e product=0x028e version=0x0114 =>
>      `"030000005e0400008e02000014010000"` com CRC zerado); `derive_layout` com bitmap
>      sintético de capacidade e asserts dos índices; parse de entrada REAL da DB do 360 e
>      resolução ponta-a-ponta (evento evdev sintético => `GamepadButton::South` true);
>      default kernel-spec num pad sem match.
>    - `gamepad_mapping_hostile.cpp`: linha sem vírgulas, GUID curto/ímpar/não-hex, campo
>      `b99999`/`a-1`/`h9.99`, linha de 1 MB, texto sem `\n` final, `parse_text(nullptr)`,
>      `~` e `+/-` malformados -- 0 entradas ou skip, nunca crash.
>    Estilo TDD red/green/refactor; asserts com mensagem; exit code != 0 em falha (padrão
>    dos testes existentes -- leia `tests/audio_hostile_sanity.cpp` como referência de forma).
> 6. Verificação local ANTES de reportar: build completo em `/var/tmp/gfx-a2-a`
>    (`export TMPDIR=/var/tmp`; `cmake -S glintfx -B /var/tmp/gfx-a2-a
>    -DGLINTFX_BUILD_TESTS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
>    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build /var/tmp/gfx-a2-a -j$(nproc)`);
>    `ctest --test-dir /var/tmp/gfx-a2-a -R gamepad --output-on-failure`;
>    clang-tidy + cppcheck conforme seção 2.7; ASan/UBSan standalone da seção 2.7 nos seus
>    4 testes. É o ÚNICO build pesado da máquina na sua janela -- não dispare outro.
> 7. Commit local (Conventional Commits pt-br, citar `A2-GAMEPAD`), **NÃO push**.
>    Convenções: SPDX em todo arquivo de código; identificadores en-intl; doc-comments
>    bilíngues; zero `#include` de terceiro em `include/glintfx/`.
>    Reporte: arquivos criados, saída do ctest, achados/limitações honestas.

### 5.2 Agente C -- `devops-sre` -- DB vendorizada + docs + ADR + conformidade CI (onda 1, paralelo a A)

> Você é o devops-sre da fatia A2-GAMEPAD do glintfx. Leia antes: o plano
> `glintfx/docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md` (seções 2.1 D6,
> 2.2, 4), `AGENTS.md`, `docs/adr/0015-...md`, `.github/workflows/{ci.yml,windows-atoms.yml}`,
> `glintfx/tools/gen_glloader.py` (padrão de gerador offline) e `NOTICE`.
> **NÃO toque em nenhum CMakeLists nem em `glintfx/src/`** (posse do agente A nesta onda).
>
> **Entregue (posse exclusiva):**
> 1. `glintfx/third_party/gamecontrollerdb/`: baixe o `gamecontrollerdb.txt` da comunidade
>    (repo `gabomdq/SDL_GameControllerDB`, licença zlib -- confirme a licença no repo),
>    filtre SÓ as entradas `platform:Linux` (+ comentário de procedência com commit/URL/data)
>    e commit como `gamecontrollerdb_linux.txt`; inclua `LICENSE` (texto zlib do upstream) e
>    `README.md` bilíngue (procedência, como re-sincronizar, por que é DADO e não dep).
> 2. `glintfx/tools/gen_gamepad_db.py`: gerador offline/reprodutível (padrão
>    `gen_glloader.py`): lê o `.txt`, valida grosseiramente as linhas, emite
>    `gamecontrollerdb_linux.inc` -- um array C `static const char
>    glintfx_gamecontrollerdb_linux[]` (string literal concatenada, chunks < 64 KB pra
>    respeitar limites de literal do C++) + `_len`. Gere e COMMITE o `.inc`. Determinístico
>    (mesma entrada => mesmo byte).
> 3. `glintfx/docs/gamepad.md` bilíngue: contrato da API (seção 2.2 do plano é a fonte),
>    exemplo de consumo no frame loop do A1 (`set_frame_callback` + `pads.poll()`),
>    **permissões** (grupo `input`, exemplo de regra udev, e a degradação graciosa:
>    permission denied => 0 pads, sem root, sem crash), mapeamento (DB embutida, formato SDL
>    como dado, default kernel-spec, `load_mappings_*`), superfície crua, limites da fatia
>    (sem rumble/força, sem deadzone imposta -- D9, sem integração App -- D10, 8 pads -- D4).
> 4. `docs/adr/0016-gamepad-atom.md` (raiz do repo, padrão dos ADRs existentes, bilíngue):
>    registra D1 do plano -- gamepad entregue como átomo próprio `GLINTFX_MODULE_GAMEPAD`;
>    `GLINTFX_MODULE_INPUT` segue reservado; refina ADR-0015 (b) sem revogá-lo; nota da
>    decisão autônoma pendente de confirmação retroativa do líder.
> 5. `NOTICE`: atribuição do SDL_GameControllerDB (zlib, dado vendorizado).
>    `CHANGELOG.md`: seção da v0.15.0 (A2-GAMEPAD) no padrão das anteriores.
>    `TODO.md`: linha A2 com status `🔍 Pendente verificação` + itens INBOX que você
>    identificar (ex.: rumble/FF, deadzone helper, integração App).
> 6. **Conformidade CI (verificação, ZERO mudança de workflow):** confirme e registre no seu
>    relatório: (a) os path-filters de `windows-atoms.yml` NÃO casam nenhum arquivo novo
>    (o job nem dispara); (b) o harness `tests/win-atoms/CMakeLists.txt` compila lista
>    explícita sem gamepad; (c) o glob `src/**/*.cpp` do clang-tidy e o cppcheck por
>    compile_commands cobrem `src/gamepad*.cpp` automaticamente. NÃO fabrique teste Windows
>    de gamepad. NÃO adicione preset (D11).
> 7. Commit local (Conventional Commits pt-br, citar `A2-GAMEPAD`), **NÃO push**. Sem build
>    pesado (o agente A detém a janela de build) -- seu trabalho é dados/docs/verificação.

### 5.3 Agente B -- `backend-engineer` (segundo, ≠ agente A) -- device layer + API pública (onda 2)

> Você é o backend-engineer da camada de device do A2-GAMEPAD. Pré-condição: onda 1 (agentes
> A e C) entregue e verificada -- os headers `src/gamepad_decode.hpp`/`src/gamepad_mapping.hpp`,
> o bloco CMake `glintfx_gamepad` e `third_party/gamecontrollerdb/gamecontrollerdb_linux.inc`
> EXISTEM. Leia antes: o plano `glintfx/docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md`
> (seções 2.2, 2.5, 2.6, 2.7), os headers do agente A, `include/glintfx/audio.hpp` +
> `src/audio.cpp` (o MODELO de contrato pimpl/RAII/hardening a espelhar), `AGENTS.md`.
>
> **Entregue:**
> 1. `glintfx/include/glintfx/gamepad.hpp` -- API pública da seção 2.2 (literal), pimpl,
>    rule-of-5 deletado com o mesmo racional documentado do `Audio`, doc-comments bilíngues
>    com o contrato completo (fail-high, slots, permissões, hotplug, copy-before-invoke do
>    callback). Guard de módulo no topo, espelho do `app.hpp`:
>    `#include <glintfx/config.hpp>` + `#if !GLINTFX_MODULE_GAMEPAD` => `#error`.
>    ZERO `#include` de terceiro (nem `<linux/...>`) neste header.
> 2. `glintfx/include/glintfx/config.hpp.in`: acrescente `#cmakedefine01 GLINTFX_MODULE_GAMEPAD`
>    com doc-comment bilíngue no padrão dos existentes.
> 3. `glintfx/src/gamepad.cpp` -- `Gamepads::Impl` (seção 2.5, literal): scan, detecção D7,
>    EACCES gracioso, ioctls de identidade/capacidade/absinfo, slots (D4), pump `poll()`
>    (read até EAGAIN, ENODEV => disconnect+callback, feed no decoder, resync
>    EVIOCGKEY/EVIOCGABS quando `sync_lost()`), inotify (`IN_CREATE|IN_ATTRIB|IN_DELETE`,
>    falha de inotify não falha init), DB embutida via `#include "gamecontrollerdb_linux.inc"`
>    gateado por `cfg.use_builtin_db`, `load_mappings_file/text` delegando ao
>    `MappingDb::parse_text` do agente A, `shutdown()` idempotente/determinístico.
> 4. CMake: acrescente `src/gamepad.cpp` à lista do OBJECT `glintfx_gamepad`
>    (`glintfx/CMakeLists.txt`); NÃO reestruture o bloco do agente A.
> 5. Testes em `glintfx/tests/` + registro no bloco `if(GLINTFX_MODULE_GAMEPAD)` de
>    `tests/CMakeLists.txt` (append; mesmo padrão dos do agente A):
>    - `gamepad_device_sanity.cpp`: `dev_dir` apontando pra (a) dir temporário vazio =>
>      `init()==true`, `connected_count()==0`; (b) dir inexistente => init ainda true, 0 pads
>      (documente); (c) dir com ARQUIVO REGULAR chamado `event0` (lixo dentro) => pulado
>      gracioso; (d) arquivo `event1` com `chmod 000` => EACCES pulado (pule o sub-caso se
>      rodando como root); shutdown+re-init suportado.
>    - `gamepad_hotplug_smoke.cpp`: init com dir temp + hotplug ON; criar/apagar arquivos
>      `eventN` entre `poll()`s => caminho do inotify executa sem crash e sem pad fantasma.
>    - `gamepad_api_hardening.cpp`: TODA a API antes de `init()` e depois de `shutdown()`
>      retorna false/0/""; `pad`/`index` negativos e >= limites; `load_mappings_file(nullptr)`,
>      arquivo inexistente, arquivo de lixo binário; `load_mappings_text(nullptr)`; segundo
>      `init()` retorna false; callback nulo é no-op seguro.
> 6. Verificação local ANTES de reportar (única janela de build pesado da máquina):
>    build completo em `/var/tmp/gfx-a2-b` (mesma linha de configure do agente A);
>    `ctest --test-dir /var/tmp/gfx-a2-b -R gamepad --output-on-failure`; DEPOIS a suíte
>    inteira: `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a
>    ctest --test-dir /var/tmp/gfx-a2-b --output-on-failure -j1` (zero regressão);
>    clang-tidy + cppcheck (seção 2.7) nos SEUS arquivos; ASan/UBSan standalone dos seus 3
>    testes (linha da seção 2.7, acrescentando `src/gamepad.cpp` e o include dir da DB).
>    Se tiver um pad físico disponível NÃO assuma -- os testes não podem depender dele.
> 7. Commit local (Conventional Commits pt-br, citar `A2-GAMEPAD`), **NÃO push**.
>    Reporte: arquivos, saída dos ctests (gamepad + suíte cheia), decisões locais tomadas.

### 5.4 Agente D -- `qa-engineer` -- review adversarial QUE EXECUTA (onda 3)

> Você é o reviewer adversarial do A2-GAMEPAD (implementer ≠ reviewer; você NÃO escreveu nada
> disso). Leia o plano `glintfx/docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md`
> inteiro e o diff completo da fatia (`git log`/`git diff` dos commits A2-GAMEPAD). Sua
> revisão EXECUTA -- relatório sem execução será rejeitado. Build em `/var/tmp/gfx-a2-rev`
> com `TMPDIR=/var/tmp`; UM build pesado por vez; se OOM, reporte em vez de thrashear.
>
> **Execute, no mínimo:**
> 1. **Rebuild limpo + suíte completa** (comandos da seção 2.7) -- zero falha, zero
>    regressão nos testes pré-existentes.
> 2. **Prova do OFF:** configure com `-DGLINTFX_MODULE_GAMEPAD=OFF`; build;
>    `nm <bld>/libglintfx.a | grep -ci gamepad` == 0; testes de gamepad ausentes do ctest;
>    suíte restante verde. Prova também que `#include <glintfx/gamepad.hpp>` com módulo OFF
>    dá o `#error` prometido (compile um snippet).
> 3. **Encapsulamento:** `./tools/check_encapsulation.sh glintfx/include/glintfx`; grep
>    manual: nenhum `#include <linux/...>` ou tipo de kernel em `gamepad.hpp`/`gamepad_types.hpp`.
> 4. **Fuzz dirigido do decoder (input hostil que VOCÊ gera, não os fixtures dos testes):**
>    escreva um harness rápido que alimente `GamepadDecoder::feed_bytes` com >= 1M de bytes
>    aleatórios (várias seeds), chunks de tamanho 1..64, sob ASan+UBSan (linha standalone da
>    seção 2.7). Zero crash/UB/OOB. Idem `MappingDb::parse_text` com texto aleatório e com
>    mutações de entradas reais da DB (troque chars, trunque no meio de campo, duplique GUIDs).
> 5. **Mutation-test do mapeamento (mínimo 4 mutantes, à mão):** inverta a ordem de
>    enumeração de botões em `derive_layout`, troque LE por BE no GUID, off-by-one no range
>    de exclusão dos hats, remova o clamp da normalização -- CADA mutante deve ser MORTO por
>    algum teste existente; mutante sobrevivente = achado (gap de teste), reporte.
> 6. **Degradação sem permissão:** rode `gamepad_device_sanity`/os cenários EACCES como
>    usuário normal; confirme zero crash, zero exigência de root, retornos documentados.
>    Se houver `/dev/input/event*` REAL na máquina: rode um smoke manual de `init()` no dir
>    real e reporte o comportamento (sem transformar isso em teste automatizado).
> 7. **Domino check (auditoria-dominó da casa):** todo guard/validação achado num caminho --
>    procure o gêmeo nos irmãos (ex.: raw_* valida índice? `guid()` valida slot? o parser
>    valida TODOS os campos ou só os testados?). Varra a superfície inteira.
> 8. **Estática:** clang-tidy + cppcheck (seção 2.7) sobre TODOS os arquivos da fatia;
>    confirme as claims do agente C sobre `windows-atoms.yml` (path-filters, harness).
> 9. **Spot-check de claims:** escolha >= 5 claims arquivo:linha dos relatórios de A/B/C e
>    verifique no código real.
>
> Classifique achados (CRITICAL/MAJOR/MINOR/NIT), com repro mínimo executável para cada
> CRITICAL/MAJOR. **NÃO corrija você mesmo; NÃO push.** Veredito final:
> APROVADO / APROVADO-COM-RESSALVAS / REPROVADO.

---

## 6. Risks / Riscos

1. **Derivação de índice compatível-SDL errada** => DB aplica mapeamento trocado em pads
   quirky. Mitigação: vetores de teste com capacidade sintética conhecida (5.1 teste 3) +
   mutation-test dedicado (5.4 item 5). É o risco técnico nº 1 da fatia.
2. **Corrida de permissão do udev no hotplug** (node criado antes do chmod) => `IN_ATTRIB`
   re-tenta; testado só por caminho de código (sem device real no CI) -- risco residual
   aceito, smoke manual do reviewer (5.4 item 6) cobre o que der.
3. **`SYN_DROPPED` sem resync** = botão fantasma; mitigado por D8 + teste da flag; o resync
   por ioctl em si só é exercitável com device real -- documentado como limite honesto.
4. **Tamanho da DB embutida** (~90-150 KB `.rodata`): aceito (D6 ⚑); `use_builtin_db=false`
   e o módulo OFF removem tudo.
5. **OOM/pressão de memória local**: 1 build pesado por vez, dirs em `/var/tmp`,
   ASan standalone (sem RmlUi) em vez de suíte ASan completa local; a suíte ASan completa
   fica com `claudio`/nightly no push.
6. **Superfície untrusted nova** (devices + arquivo de DB) -- gatilho (d) do
   `.bigtech-porte` (Narciso/CISO): coberto pelos testes hostis + fuzz do reviewer; se o
   reviewer achar CRITICAL de parsing, considerar rodada extra de `security-engineer`.

## 7. Scope recommendation / Recomendação de escopo (fatia mínima defensável)

**Dentro:** 8 pads, hotplug inotify (D3), layout normalizado completo + superfície crua
completa, parser SDL-db + DB Linux embutida + `load_mappings_*`, default kernel-spec,
degradação graciosa de permissão, docs + ADR-0016. **Fora (INBOX):** rumble/force-feedback
(`EV_FF`), helper de deadzone, integração `App`/`AppConfig`, aceitação de joysticks de voo
como categoria testada (a detecção D7 já os aceita; qualidade de mapeamento deles não é
promessa desta fatia), preset `gamepad-only` (bloqueado por `GLINTFX_MODULE_UI` não existir).

## 8. Open questions for the leader / Perguntas ao líder (nenhuma bloqueia o início)

1. ⚑ D6: confirmar retroativamente a vendorização embutida do subconjunto Linux da
   SDL_GameControllerDB (~90-150 KB de `.rodata` com o módulo ON; alternativa era só
   parser + arquivo externo).
2. ⚑ D1/ADR-0016: confirmar retroativamente o átomo próprio `GLINTFX_MODULE_GAMEPAD`
   (vs. dobrar no futuro `GLINTFX_MODULE_INPUT`).
