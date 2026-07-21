// SPDX-License-Identifier: MPL-2.0
// EN: A2-GAMEPAD (framework-2D, ADR-0015/ADR-0016 module "gamepad") -- the WHOLE public surface
//     of the module: pimpl, RAII, same contract shape as `Audio` (glintfx/audio.hpp) -- depends
//     on `core` ONLY (no GL, no window, no RmlUi type crossing this header), testable without
//     Xvfb. UNLIKE `Audio`, this module is **Linux-only by nature**: it is a thin, hostile-input-
//     hardened facade over raw evdev (`/dev/input/eventN`), a Linux kernel interface with no
//     portable substitute -- see the `#error` guard immediately below and `GLINTFX_MODULE_GAMEPAD`'s
//     own CMake option doc-string (CMakeLists.txt) for the platform gate.
//
//     ARCHITECTURE: this header/`.cpp` (`src/gamepad.cpp`) is the DEVICE LAYER -- directory scan
//     of `dev_dir`, capability-based gamepad detection, `open()`/`ioctl()`/`poll()`/`inotify`,
//     slot bookkeeping. It sits on top of two PURE, headless-testable modules this header never
//     names: the decode core (`src/gamepad_decode.hpp`, byte-stream reassembly + per-code
//     button/axis state) and the mapping module (`src/gamepad_mapping.hpp`, SDL-GUID + DB parser +
//     kernel-spec default). No third-party/kernel type (`input_event`/`input_id`/`<linux/input.h>`)
//     crosses THIS boundary -- `gamepad_types.hpp`'s plain `GamepadButton`/`GamepadAxis` enums are
//     the only vocabulary a consumer needs.
//
//     LIFECYCLE / RAII (mirrors `Audio`'s contract, see that header's own comment for the full
//     rationale): `Gamepads()` touches nothing (no fd, no ioctl, no inotify watch) -- construction
//     is always cheap and side-effect-free. `init(GamepadsConfig)` performs the initial directory
//     scan and (if `hotplug`) sets up the inotify watch; it returns `true` even with ZERO pads
//     connected (an empty `dev_dir`, a `dev_dir` with no gamepad-shaped nodes, or every node
//     permission-denied are all NORMAL, not-yet-connected states, not init failures) -- it returns
//     `false` ONLY when this instance is ALREADY initialized (call `shutdown()` first to re-init;
//     a `shutdown()` + `init()` cycle is fully supported). `shutdown()` is idempotent and
//     deterministic: every open pad fd and the inotify fd are closed, every slot is cleared. The
//     destructor calls `shutdown()`. Every operation performed before `init()` succeeds or after
//     `shutdown()` returns `false`/`0`/`""` -- never a crash (same fail-high discipline as
//     `Audio`/`UiLayer`/`decorator_polygon.cpp`'s hostile-input handling).
//
//     FRAME-LOOP INTEGRATION (D10 of the A2-GAMEPAD plan -- no `App`/`AppConfig` coupling in this
//     slice): a consumer owns a `Gamepads` instance and calls `poll()` once per frame, typically
//     from the SAME place it already calls `App::set_frame_callback`'s callback (A1) --
//     `poll()` is the ONLY function that ever touches a file descriptor after `init()`; every
//     other query below reads already-decoded, already-normalized in-memory state.
//
//     SLOTS (D4): up to `kMaxPads` (8) pads are tracked concurrently in fixed, STABLE slots --
//     a connected pad keeps its slot index for as long as it stays connected; on disconnect the
//     slot becomes free and the NEXT hotplugged pad reuses the LOWEST free slot index (not
//     necessarily the one the disconnected pad had -- "lowest free", not "same as before").
//     A 9th simultaneously-connected pad is silently ignored (no slot, no callback) until a slot
//     frees up -- this is a documented capacity limit, not a crash.
//
//     PERMISSIONS (graceful EACCES degradation, see `docs/gamepad.md` for the udev/`input`-group
//     detail): a `/dev/input/eventN` node this process cannot `open(O_RDONLY)` (no root, not in
//     the `input` group, no matching udev rule) is silently SKIPPED at scan time and at hotplug
//     time -- it never becomes a connected pad, never crashes `init()`/`poll()`, and is not
//     reported as an error (no per-frame log spam). `connected_count() == 0` after a successful
//     `init()` is consequently a legitimate, expected state on a locked-down host.
//
//     HOTPLUG (D2/D3): when `GamepadsConfig::hotplug` is `true` (the default), `init()` watches
//     `dev_dir` via `inotify` (`IN_CREATE`/`IN_ATTRIB`/`IN_DELETE` -- `IN_ATTRIB` specifically
//     catches the udev permission-race, where the device NODE appears slightly before udev's
//     `chmod`/group-assignment rule finishes applying to it, so a plain `IN_CREATE`-triggered
//     `open()` attempt would still see EACCES; the FOLLOW-UP `IN_ATTRIB` re-attempts it once the
//     permissions have actually settled). `poll()` is what drains the inotify fd and reacts to it
//     -- nothing happens between `poll()` calls. If `inotify_init1`/`inotify_add_watch` fails
//     (e.g. `dev_dir` does not exist yet), `init()` still returns `true`: hotplug silently degrades
//     to "the pads seen at `init()` time only", the initial scan's result is unaffected.
//
//     THE NORMALIZED VS. RAW SURFACE (the plan's own leader decision -- "expose ALL raw buttons/
//     axes, not just the standard layout"): `button()`/`axis()` return the NORMALIZED
//     `GamepadButton`/`GamepadAxis` vocabulary (resolved via the loaded mapping database or the
//     kernel-spec default, see `gamepad_types.hpp`); `raw_*()` expose the COMPLETE underlying
//     control surface a pad's capability bitmap reports, indexed `[0, raw_*_count(pad))`, with NO
//     interpretation applied beyond the decode core's own per-axis normalization (`raw_axis()`) --
//     useful for flight-stick-shaped devices D7's capability check happily accepts but the
//     kernel-gamepad-spec-shaped normalized layout was never promised to describe well (see the
//     plan's own scope note, section 7).
//
//     FAIL-HIGH CONVENTION (the house rule, same as every other module -- `Audio`/`UiLayer`/
//     `I18n`): every accessor below with a `pad` or `index` argument outside its valid range, OR
//     called before `init()` succeeds / after `shutdown()`, returns `false`/`0`/`0.0f`/`""` --
//     NEVER undefined behaviour, never a crash. `set_connection_callback()`'s callback is invoked
//     with a LOCAL COPY of the stored `std::function` (AUD-TEC-3 reentrancy discipline, same as
//     every other callback in this library, e.g. `Bootstrap`'s click/hover listeners,
//     `bootstrap.cpp`) -- a callback that itself calls `set_connection_callback(...)` (swapping in
//     a new handler) cannot destroy the `std::function` object out from under the invocation
//     currently running it. A `nullptr`/empty callback is a safe no-op (nothing is invoked, no
//     crash) -- `poll()` simply skips the call when no callback is set.
// PT: A2-GAMEPAD (framework-2D, módulo "gamepad" da ADR-0015/ADR-0016) -- a superfície pública
//     INTEIRA do módulo: pimpl, RAII, mesma forma de contrato do `Audio` (glintfx/audio.hpp) --
//     depende só de `core` (zero GL, zero janela, zero tipo do RmlUi cruzando este header),
//     testável sem Xvfb. DIFERENTE do `Audio`, este módulo é **Linux-only por natureza**: é uma
//     fachada fina, endurecida contra input hostil, sobre evdev cru (`/dev/input/eventN`), uma
//     interface do kernel Linux sem substituto portável -- ver a guarda `#error` logo abaixo e a
//     própria doc-string da option `GLINTFX_MODULE_GAMEPAD` do CMake (CMakeLists.txt) pro gate de
//     plataforma.
//
//     ARQUITETURA: este header/`.cpp` (`src/gamepad.cpp`) é a CAMADA DE DEVICE -- varredura de
//     diretório de `dev_dir`, detecção de gamepad por capacidade, `open()`/`ioctl()`/`poll()`/
//     `inotify`, contabilidade de slot. Fica em cima de dois módulos PUROS, testáveis headless,
//     que este header nunca nomeia: o núcleo de decode (`src/gamepad_decode.hpp`, reagrupamento de
//     byte-stream + estado de botão/eixo por código) e o módulo de mapeamento
//     (`src/gamepad_mapping.hpp`, GUID SDL + parser de DB + default kernel-spec). Nenhum tipo de
//     terceiro/kernel (`input_event`/`input_id`/`<linux/input.h>`) cruza ESTA fronteira --
//     os enums simples `GamepadButton`/`GamepadAxis` do `gamepad_types.hpp` são todo o vocabulário
//     que um consumidor precisa.
//
//     CICLO DE VIDA / RAII (espelha o contrato do `Audio`, ver o próprio comentário daquele header
//     pro racional completo): `Gamepads()` não toca em nada (sem fd, sem ioctl, sem watch de
//     inotify) -- a construção é sempre barata e sem efeito colateral. `init(GamepadsConfig)`
//     realiza a varredura inicial de diretório e (se `hotplug`) monta o watch de inotify; retorna
//     `true` mesmo com ZERO pads conectados (um `dev_dir` vazio, um `dev_dir` sem nenhum node com
//     formato de gamepad, ou todo node com permissão negada são estados NORMAIS de
//     "ainda-não-conectado", não falhas de init) -- retorna `false` SÓ quando esta instância JÁ
//     está inicializada (chame `shutdown()` primeiro pra re-inicializar; um ciclo `shutdown()` +
//     `init()` é totalmente suportado). `shutdown()` é idempotente e determinístico: todo fd de pad
//     aberto e o fd de inotify são fechados, todo slot é limpo. O destrutor chama `shutdown()`.
//     Toda operação feita antes de `init()` ter sucesso ou depois de `shutdown()` retorna
//     `false`/`0`/`""` -- nunca um crash (mesma disciplina fail-high do
//     `Audio`/`UiLayer`/tratamento de input hostil do `decorator_polygon.cpp`).
//
//     INTEGRAÇÃO NO FRAME LOOP (D10 do plano A2-GAMEPAD -- sem acoplamento a `App`/`AppConfig`
//     nesta fatia): um consumidor possui uma instância `Gamepads` e chama `poll()` uma vez por
//     frame, tipicamente do MESMO lugar onde já chama o callback do `App::set_frame_callback`
//     (A1) -- `poll()` é a ÚNICA função que toca um descritor de arquivo depois do `init()`; toda
//     outra consulta abaixo lê estado já decodificado e já normalizado, em memória.
//
//     SLOTS (D4): até `kMaxPads` (8) pads são rastreados simultaneamente em slots fixos e
//     ESTÁVEIS -- um pad conectado mantém seu índice de slot enquanto continuar conectado; ao
//     desconectar o slot fica livre e o PRÓXIMO pad conectado via hotplug reusa o MENOR índice de
//     slot livre (não necessariamente o que o pad desconectado tinha -- "menor livre", não "o
//     mesmo de antes"). Um 9º pad conectado simultaneamente é silenciosamente ignorado (sem slot,
//     sem callback) até um slot liberar -- isto é um limite de capacidade documentado, não um
//     crash.
//
//     PERMISSÕES (degradação graciosa de EACCES, ver `docs/gamepad.md` pro detalhe de udev/grupo
//     `input`): um node `/dev/input/eventN` que este processo não consegue `open(O_RDONLY)` (sem
//     root, fora do grupo `input`, sem regra udev correspondente) é silenciosamente PULADO em
//     tempo de varredura e em tempo de hotplug -- nunca vira um pad conectado, nunca crasha
//     `init()`/`poll()`, e não é reportado como erro (sem spam de log por frame).
//     `connected_count() == 0` depois de um `init()` bem-sucedido é consequentemente um estado
//     legítimo e esperado num host travado.
//
//     HOTPLUG (D2/D3): quando `GamepadsConfig::hotplug` é `true` (o padrão), `init()` vigia
//     `dev_dir` via `inotify` (`IN_CREATE`/`IN_ATTRIB`/`IN_DELETE` -- `IN_ATTRIB` especificamente
//     pega a corrida de permissão do udev, onde o NODE de device aparece um pouco antes da regra
//     de `chmod`/atribuição-de-grupo do udev terminar de se aplicar a ele, então uma tentativa de
//     `open()` disparada só por `IN_CREATE` ainda veria EACCES; o `IN_ATTRIB` de ACOMPANHAMENTO
//     tenta de novo assim que as permissões de fato se acomodaram). `poll()` é quem drena o fd de
//     inotify e reage a ele -- nada acontece entre chamadas de `poll()`. Se `inotify_init1`/
//     `inotify_add_watch` falhar (ex.: `dev_dir` ainda não existe), `init()` ainda retorna `true`:
//     o hotplug degrada silenciosamente pra "só os pads vistos em tempo de `init()`", o resultado
//     da varredura inicial não é afetado.
//
//     SUPERFÍCIE NORMALIZADA VS. CRUA (a própria decisão do líder do projeto -- "expor TODOS os
//     botões/eixos crus, não só o layout padrão"): `button()`/`axis()` retornam o vocabulário
//     NORMALIZADO `GamepadButton`/`GamepadAxis` (resolvido via a base de mapeamento carregada ou o
//     default kernel-spec, ver `gamepad_types.hpp`); `raw_*()` expõem a superfície de controle
//     COMPLETA que o bitmap de capacidade de um pad reporta, indexada `[0, raw_*_count(pad))`, sem
//     NENHUMA interpretação além da própria normalização por-eixo do núcleo de decode
//     (`raw_axis()`) -- útil pra devices com formato de manche de voo que a checagem de capacidade
//     D7 aceita de bom grado mas que o layout normalizado com formato de spec-de-gamepad-do-kernel
//     nunca prometeu descrever bem (ver a própria nota de escopo do plano, seção 7).
//
//     CONVENÇÃO FAIL-HIGH (a regra da casa, igual a todo outro módulo -- `Audio`/`UiLayer`/
//     `I18n`): todo acessor abaixo com argumento `pad` ou `index` fora da faixa válida, OU chamado
//     antes de `init()` ter sucesso / depois de `shutdown()`, retorna `false`/`0`/`0.0f`/`""` --
//     NUNCA comportamento indefinido, nunca um crash. O callback de `set_connection_callback()` é
//     invocado com uma CÓPIA LOCAL do `std::function` armazenado (disciplina de reentrância
//     AUD-TEC-3, igual a todo outro callback desta biblioteca, ex.: os listeners de clique/hover
//     do `Bootstrap`, `bootstrap.cpp`) -- um callback que ele mesmo chama
//     `set_connection_callback(...)` (trocando por um handler novo) não consegue destruir o objeto
//     `std::function` debaixo da invocação que está rodando ele agora. Um callback `nullptr`/vazio
//     é um no-op seguro (nada é invocado, sem crash) -- `poll()` simplesmente pula a chamada quando
//     nenhum callback está setado.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <glintfx/config.hpp>
#if !GLINTFX_MODULE_GAMEPAD
#error "glintfx::Gamepads requires GLINTFX_MODULE_GAMEPAD=ON (Linux-only module: raw evdev; see ADR-0016)"
#endif

#include <glintfx/gamepad_types.hpp>

#include <cstdint>
#include <functional>
#include <memory>

namespace glintfx {

// EN: Configuration for Gamepads::init(). Declared at namespace scope (default-argument rule,
//     same reason as AudioConfig/UiLayerConfig).
// PT: Configuração para Gamepads::init(). Declarada em namespace scope (regra de argumento
//     padrão, mesmo motivo do AudioConfig/UiLayerConfig).
struct GamepadsConfig {
  // EN: Directory scanned for `eventN` nodes and (if `hotplug`) watched via inotify. The default
  //     `"/dev/input"` is the real Linux evdev directory; tests override this to a temporary,
  //     headless-safe directory (empty, containing garbage files, permission-denied files -- see
  //     tests/gamepad_device_sanity.cpp) so this module's own test suite never touches the host's
  //     real input devices. A `nullptr` value is treated exactly like the default (never
  //     dereferenced as-is).
  // PT: Diretório varrido por nodes `eventN` e (se `hotplug`) vigiado via inotify. O padrão
  //     `"/dev/input"` é o diretório evdev real do Linux; os testes sobrescrevem isto pra um
  //     diretório temporário, seguro pra headless (vazio, contendo arquivos-lixo, arquivos com
  //     permissão negada -- ver tests/gamepad_device_sanity.cpp), então a própria suíte de teste
  //     deste módulo nunca toca nos devices de input reais do host. Um valor `nullptr` é tratado
  //     exatamente como o padrão (nunca desreferenciado como está).
  const char* dev_dir = "/dev/input";

  // EN: `true` (default): `init()` also sets up an inotify watch on `dev_dir` so `poll()` can
  //     detect pads plugged/unplugged after `init()` ran (D2/D3 of the plan -- see this file's
  //     header comment). `false`: only the pads present at `init()` time are ever seen; `poll()`
  //     still pumps already-connected pads' input, it just never grows/shrinks the connected set.
  // PT: `true` (padrão): `init()` também monta um watch de inotify em `dev_dir` pra que `poll()`
  //     detecte pads plugados/desplugados depois do `init()` rodar (D2/D3 do plano -- ver o
  //     comentário de cabeçalho deste arquivo). `false`: só os pads presentes em tempo de
  //     `init()` são vistos; `poll()` continua bombeando o input dos pads já conectados, só nunca
  //     cresce/encolhe o conjunto de conectados.
  bool hotplug = true;

  // EN: `true` (default): the vendored Linux subset of the community `gamecontrollerdb.txt`
  //     (D6 of the plan; `third_party/gamecontrollerdb/`) is merged in at `init()` time, before
  //     any `load_mappings_*()` call a consumer makes. `false`: start with an EMPTY mapping
  //     database -- every pad falls back to `default_kernel_spec_mapping()` (D5) unless a
  //     consumer calls `load_mappings_file()`/`load_mappings_text()` itself.
  // PT: `true` (padrão): o subconjunto Linux vendorizado do `gamecontrollerdb.txt` da comunidade
  //     (D6 do plano; `third_party/gamecontrollerdb/`) é mesclado em tempo de `init()`, antes de
  //     qualquer chamada `load_mappings_*()` que um consumidor faça. `false`: começa com uma base
  //     de mapeamento VAZIA -- todo pad cai no `default_kernel_spec_mapping()` (D5) a menos que um
  //     consumidor chame `load_mappings_file()`/`load_mappings_text()` por conta própria.
  bool use_builtin_db = true;
};

// EN: Raw-evdev gamepad facade. See this file's header comment above for the full lifecycle/
//     RAII/slots/permissions/hotplug/fail-high contract. No third-party (kernel evdev) type
//     crosses this header -- `Impl` (gamepad.cpp) is where `input_event`/`input_id`/ioctl numbers
//     actually live.
// PT: Fachada de gamepad via evdev cru. Ver o comentário de cabeçalho deste arquivo acima para o
//     contrato completo de ciclo-de-vida/RAII/slots/permissões/hotplug/fail-high. Nenhum tipo de
//     terceiro (evdev do kernel) cruza este header -- `Impl` (gamepad.cpp) é onde
//     `input_event`/`input_id`/números de ioctl de fato vivem.
class Gamepads {
public:
  // EN: Fixed slot count (D4 of the plan) -- see this file's header comment for the slot-reuse
  //     policy ("lowest free", not "same as before").
  // PT: Contagem de slot fixa (D4 do plano) -- ver o comentário de cabeçalho deste arquivo pra
  //     política de reuso de slot ("menor livre", não "o mesmo de antes").
  static constexpr int kMaxPads = 8;

  // EN: Does NOT touch dev_dir/fds/inotify -- construction is always cheap and side-effect-free;
  //     the real resource acquisition happens in init().
  // PT: NÃO toca em dev_dir/fds/inotify -- a construção é sempre barata e sem efeito colateral; a
  //     aquisição do recurso de verdade acontece em init().
  Gamepads();

  // EN: Deterministic: calls shutdown() (safe even if init() was never called or already shut
  //     down -- shutdown() is idempotent).
  // PT: Determinístico: chama shutdown() (seguro mesmo se init() nunca foi chamado ou já foi
  //     desligado -- shutdown() é idempotente).
  ~Gamepads();

  // EN: Rule-of-5, explicit (same racional as Audio's, see audio.hpp): a user-declared destructor
  //     suppresses the implicit move members, so deleting copy alone would leave move
  //     ambiguous/half-declared. Copying a `Gamepads` makes no sense (it owns live fds and an
  //     inotify watch -- no sane "duplicate this open device set" semantics); moving one out from
  //     under itself is unsafe by the same RAII-ordering argument as shutdown() -- revisit only if
  //     a real use case asks for it.
  // PT: Regra dos 5, explícita (mesmo racional do Audio, ver audio.hpp): um destrutor declarado
  //     pelo usuário suprime os membros de move implícitos, então só deletar o copy deixaria o
  //     move ambíguo/meio-declarado. Copiar um `Gamepads` não faz sentido (ele é dono de fds vivos
  //     e um watch de inotify -- não há semântica sã de "duplicar este conjunto de devices
  //     abertos"); mover um de baixo de si mesmo é inseguro pelo mesmo argumento de ordem de RAII
  //     do shutdown() -- revisitar só se um caso de uso real pedir.
  Gamepads(const Gamepads&)            = delete;
  Gamepads& operator=(const Gamepads&) = delete;
  Gamepads(Gamepads&&)                 = delete;
  Gamepads& operator=(Gamepads&&)      = delete;

  // EN: Scans `cfg.dev_dir` once and (if `cfg.hotplug`) sets up the inotify watch. Returns `true`
  //     even with 0 pads found/accessible (see this file's header comment) -- `false` ONLY when
  //     this instance is already initialized (call shutdown() first to re-init).
  // PT: Varre `cfg.dev_dir` uma vez e (se `cfg.hotplug`) monta o watch de inotify. Retorna `true`
  //     mesmo com 0 pads encontrados/acessíveis (ver o comentário de cabeçalho deste arquivo) --
  //     `false` SÓ quando esta instância já está inicializada (chame shutdown() primeiro pra
  //     re-inicializar).
  bool init(const GamepadsConfig& cfg = {});

  // EN: Idempotent, deterministic teardown: every open pad fd and the inotify fd (if any) are
  //     closed, every slot cleared. Safe to call any number of times, including on a
  //     never-initialized instance (no-op). After this call every query below returns
  //     false/0/0.0f/"" until the next init().
  // PT: Teardown idempotente e determinístico: todo fd de pad aberto e o fd de inotify (se
  //     houver) são fechados, todo slot é limpo. Seguro chamar quantas vezes for, inclusive numa
  //     instância nunca inicializada (no-op). Depois desta chamada toda consulta abaixo retorna
  //     false/0/0.0f/"" até o próximo init().
  void shutdown();

  // EN: true between a successful init() and the next shutdown()/destruction.
  // PT: true entre um init() bem-sucedido e o próximo shutdown()/destruição.
  bool is_initialized() const;

  // EN: Per-frame pump (see this file's header comment for the frame-loop-integration contract):
  //     drains every connected pad's fd (read() in a loop until EAGAIN), resyncs a pad's full
  //     key/abs state via EVIOCGKEY/EVIOCGABS when the decode core flags SYN_DROPPED (D8), then
  //     drains and reacts to the inotify fd (if hotplug is on). No-op (does nothing, no crash)
  //     when not initialized.
  // PT: Bombeamento por-frame (ver o comentário de cabeçalho deste arquivo pro contrato de
  //     integração no frame loop): drena o fd de todo pad conectado (read() em laço até EAGAIN),
  //     ressincroniza o estado completo de tecla/abs de um pad via EVIOCGKEY/EVIOCGABS quando o
  //     núcleo de decode sinaliza SYN_DROPPED (D8), depois drena e reage ao fd de inotify (se o
  //     hotplug estiver ligado). No-op (não faz nada, sem crash) quando não inicializado.
  void poll();

  // EN: Number of currently connected pads, in `[0, kMaxPads]`. `0` when not initialized.
  // PT: Número de pads conectados no momento, em `[0, kMaxPads]`. `0` quando não inicializado.
  int connected_count() const;

  // EN: `pad` outside `[0, kMaxPads)`, or not initialized: returns `false`.
  // PT: `pad` fora de `[0, kMaxPads)`, ou não inicializado: retorna `false`.
  bool connected(int pad) const;

  // EN: The kernel-reported device name (EVIOCGNAME). `""` when `pad` is out of range,
  //     disconnected, or not initialized.
  // PT: O nome de device reportado pelo kernel (EVIOCGNAME). `""` quando `pad` está fora de
  //     faixa, desconectado, ou não inicializado.
  const char* name(int pad) const;

  // EN: The 32-lower-case-hex-char SDL-format GUID (see src/gamepad_mapping.hpp's build_guid()).
  //     `""` when `pad` is out of range, disconnected, or not initialized.
  // PT: O GUID em formato SDL de 32 chars hex minúsculos (ver o build_guid() do
  //     src/gamepad_mapping.hpp). `""` quando `pad` está fora de faixa, desconectado, ou não
  //     inicializado.
  const char* guid(int pad) const;

  // --- normalized layout (via loaded DB or default kernel-spec) ---
  // --- layout normalizado (via DB carregada ou default kernel-spec) ---

  // EN: `false` when `pad` is out of range, disconnected, not initialized, or `b` has no mapping
  //     for this pad (an unmapped control never guesses -- see GamepadMapping's own doc-comment,
  //     src/gamepad_mapping.hpp).
  // PT: `false` quando `pad` está fora de faixa, desconectado, não inicializado, ou `b` não tem
  //     mapeamento pra este pad (um controle não mapeado nunca chuta -- ver o próprio doc-comment
  //     do GamepadMapping, src/gamepad_mapping.hpp).
  bool button(int pad, GamepadButton b) const;

  // EN: Sticks normalized to `[-1, 1]`, triggers to `[0, 1]`. `0.0f` when `pad` is out of range,
  //     disconnected, not initialized, or `a` has no mapping for this pad.
  // PT: Analógicos normalizados para `[-1, 1]`, gatilhos para `[0, 1]`. `0.0f` quando `pad` está
  //     fora de faixa, desconectado, não inicializado, ou `a` não tem mapeamento pra este pad.
  float axis(int pad, GamepadAxis a) const;

  // --- complete RAW surface (leader decision: never restrict to the normalized layout) ---
  // --- superfície CRUA completa (decisão do líder: nunca restringir ao layout normalizado) ---

  // EN: Number of raw buttons this pad's capability bitmap reports (see derive_layout(),
  //     src/gamepad_mapping.hpp). `0` when `pad` is out of range, disconnected, or not
  //     initialized.
  // PT: Número de botões crus que o bitmap de capacidade deste pad reporta (ver derive_layout(),
  //     src/gamepad_mapping.hpp). `0` quando `pad` está fora de faixa, desconectado, ou não
  //     inicializado.
  int raw_button_count(int pad) const;

  // EN: `index` outside `[0, raw_button_count(pad))`: returns `false`.
  // PT: `index` fora de `[0, raw_button_count(pad))`: retorna `false`.
  bool raw_button(int pad, int index) const;

  // EN: The raw evdev `BTN_*`/`KEY_*` code backing `index`. `0` (never a valid evdev code this
  //     module would enumerate here) when `index` is out of range.
  // PT: O código evdev `BTN_*`/`KEY_*` cru por trás de `index`. `0` (nunca um código evdev válido
  //     que este módulo enumeraria aqui) quando `index` está fora de faixa.
  std::uint16_t raw_button_code(int pad, int index) const;

  // EN: Number of raw axes this pad's capability bitmap reports, EXCLUDING hats (see
  //     derive_layout()). `0` when `pad` is out of range, disconnected, or not initialized.
  // PT: Número de eixos crus que o bitmap de capacidade deste pad reporta, EXCLUINDO hats (ver
  //     derive_layout()). `0` quando `pad` está fora de faixa, desconectado, ou não inicializado.
  int raw_axis_count(int pad) const;

  // EN: Normalized (via the decode core's own absinfo-based formula, see gamepad_decode.hpp).
  //     `0.0f` when `index` is out of range.
  // PT: Normalizado (via a própria fórmula baseada em absinfo do núcleo de decode, ver
  //     gamepad_decode.hpp). `0.0f` quando `index` está fora de faixa.
  float raw_axis(int pad, int index) const;

  // EN: Raw, un-normalized last-reported evdev value. `0` when `index` is out of range.
  // PT: Valor cru, não-normalizado, do último evdev reportado. `0` quando `index` está fora de
  //     faixa.
  std::int32_t raw_axis_value(int pad, int index) const;

  // EN: The raw evdev `ABS_*` code backing `index`. `0` when `index` is out of range -- BUT,
  //     unlike `raw_button_code()` above, `0` is ALSO `ABS_X` itself (the most common analog
  //     stick axis), a perfectly real, frequently-enumerated evdev code -- there is no unused
  //     "0 means invalid" sentinel value in the `ABS_*` code space the way `KEY_RESERVED == 0`
  //     gives one for `BTN_*`/`KEY_*`. A consumer MUST NOT treat a `0` return from this function
  //     as "index was invalid": the only reliable validity gate is `index` being in
  //     `[0, raw_axis_count(pad))` BEFORE calling this (checked via `raw_axis_count()`), never
  //     the return value itself (review-adversarial finding, A2-GAMEPAD).
  // PT: O código evdev `ABS_*` cru por trás de `index`. `0` quando `index` está fora de faixa --
  //     MAS, diferente do `raw_button_code()` acima, `0` TAMBÉM é o próprio `ABS_X` (o eixo de
  //     stick analógico mais comum), um código evdev perfeitamente real e frequentemente
  //     enumerado -- não existe um valor-sentinela "0 significa inválido" não-usado no espaço de
  //     códigos `ABS_*` do jeito que `KEY_RESERVED == 0` dá pro `BTN_*`/`KEY_*`. Um consumidor
  //     NÃO PODE tratar um retorno `0` desta função como "index era inválido": o único portão de
  //     validade confiável é `index` estar em `[0, raw_axis_count(pad))` ANTES de chamar isto
  //     (checado via `raw_axis_count()`), nunca o próprio valor de retorno (achado do review
  //     adversarial, A2-GAMEPAD).
  std::uint16_t raw_axis_code(int pad, int index) const;

  // --- mappings ---
  // --- mapeamentos ---

  // EN: Reads `path` and parses it as gamecontrollerdb.txt-format text (MappingDb::parse_text(),
  //     src/gamepad_mapping.hpp), merging matched entries into this instance's mapping database
  //     (last-write-wins per GUID). Returns the number of entries added/replaced -- `0` when not
  //     initialized, `path` is `nullptr`, the file cannot be opened/read, or it contains zero
  //     valid `platform:Linux` entries. Never crashes on a hostile/binary-garbage file.
  // PT: Lê `path` e faz parse dele como texto em formato gamecontrollerdb.txt
  //     (MappingDb::parse_text(), src/gamepad_mapping.hpp), mesclando as entradas casadas na base
  //     de mapeamento desta instância (último-escreve-vence por GUID). Retorna o número de
  //     entradas adicionadas/substituídas -- `0` quando não inicializado, `path` é `nullptr`, o
  //     arquivo não pode ser aberto/lido, ou ele contém zero entradas `platform:Linux` válidas.
  //     Nunca crasha num arquivo hostil/lixo binário.
  int load_mappings_file(const char* path);

  // EN: Same as load_mappings_file(), but `text` is already in memory. `0` when not initialized
  //     or `text` is `nullptr`.
  // PT: Igual a load_mappings_file(), mas `text` já está em memória. `0` quando não inicializado
  //     ou `text` é `nullptr`.
  int load_mappings_text(const char* text);

  // --- connection events (optional) ---
  // --- eventos de conexão (opcional) ---

  // EN: `cb(pad, connected)` is invoked from inside poll() whenever a pad is newly connected
  //     (`connected == true`) or disconnected (`connected == false`) -- see this file's header
  //     comment for the mandatory copy-before-invoke reentrancy discipline. An empty/`nullptr`
  //     `std::function` is a safe no-op (nothing invoked from poll()).
  // PT: `cb(pad, connected)` é invocado de dentro do poll() sempre que um pad é recém-conectado
  //     (`connected == true`) ou desconectado (`connected == false`) -- ver o comentário de
  //     cabeçalho deste arquivo pra disciplina obrigatória de cópia-antes-de-invocar por
  //     reentrância. Um `std::function` vazio/`nullptr` é um no-op seguro (nada é invocado a
  //     partir do poll()).
  void set_connection_callback(std::function<void(int pad, bool connected)> cb);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
