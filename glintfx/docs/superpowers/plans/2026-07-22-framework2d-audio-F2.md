# AUDIO-F2 — Loop, Fade, Polyphony, is_playing (consumer-driven, GusWorld) — target v0.18.0

> EN first, PT below (house convention). Plan author: Caetano (CTO). Mode: autonomous, small
> workflow (< 5 agents). Item ID for commits: `AUDIO-F2` (sub-items `AUDIO-F2.1..3`).

---

## Part A — EN

### 0. Why / scope

The GusWorld consumer is migrating from direct miniaudio to `glintfx::Audio` (A3-AUDIO, v0.14).
The RAII/deterministic-shutdown contract is exactly what they wanted, but four behaviours they
already have in production are missing — switching today would REGRESS their audio. All four map
to native miniaudio calls (verified against the vendored `third_party/miniaudio/miniaudio.h`):

1. **Gapless loop** (critical) — `ma_sound_set_looping` (decl. line ~11462).
2. **Fade in/out** — `ma_sound_set_fade_in_milliseconds` (~11449) +
   `ma_sound_stop_with_fade_in_milliseconds` (~11403). ⚠ The DECLARATION's parameter is
   misnamed `fadeLengthInFrames`; the definition (~78811) takes MILLISECONDS and converts via
   the engine sample rate. Trust the definition.
3. **SFX polyphony** — `ma_sound_init_copy` (~11393) per overlapping play + lazy reap via
   `ma_sound_at_end` (~11464). Currently declared-deferred in audio.hpp.
4. **`is_playing(SoundId)`** — `ma_sound_is_playing` (~11459); 3+ headless GusWorld tests need it.

Out of scope (recorded, non-blocking): streaming (`MA_SOUND_FLAG_STREAM`) → INBOX; sound
groups/buses (consumer emulates with per-sound × master volume); OGG.

### 1. API surface (decided)

Backwards compatible at source level — existing `play(id)` / `stop(id)` calls keep compiling and
keep their exact semantics (single "primary" instance per SoundId, restart from frame 0).

```cpp
// Extended (defaulted params — source-compatible):
bool play(SoundId id, bool loop = false, float fade_in_s = 0.0f);
bool stop(SoundId id, float fade_out_s = 0.0f);

// New:
bool set_looping(SoundId id, bool loop);          // live toggle on the primary instance
bool play_oneshot(SoundId id);                    // fire-and-forget overlapping copy, never loops
bool is_playing(SoundId id) const;                // true if primary OR any live copy is playing
std::uint32_t voice_count(SoundId id) const;      // # of instances currently playing (primary + copies)
```

Semantics (the contract, to be doc-commented bilingually in audio.hpp):

- `play(id, loop, fade_in_s)`: acts on the PRIMARY instance only. Always seeks to frame 0
  (unchanged restart contract). MUST call `ma_sound_reset_stop_time` + `ma_sound_reset_fade`
  (or the combined reset if present in the vendored header) BEFORE seek+start — otherwise a
  previously scheduled `stop(id, fade)` silently kills the new playback (miniaudio's own header
  comment warns about this; it is a mandatory regression test, see §3).
  Fade-in: `ma_sound_set_fade_in_milliseconds(snd, 0.0f, END, ms)` where `END` is the sound's
  CURRENT volume (`ma_sound_get_volume`), so `set_volume(id, 0.5f)` survives a fade-in
  (implementer must verify: `-1` sentinel semantics differ between begin/end args — do not
  guess, read the vendored implementation).
- `stop(id, fade_out_s)`: `fade_out_s == 0` → immediate `ma_sound_stop` (today's behaviour).
  `> 0` → `ma_sound_stop_with_fade_in_milliseconds` on the primary AND on every live copy of
  that id. While fading out, `is_playing` stays true (document it — that is the observable).
- `set_looping(id, loop)`: primary only, applies live. Copies NEVER loop (a looping orphan
  copy could never be reaped — deliberate footgun removal, documented).
- `play_oneshot(id)`: creates a copy via `ma_sound_init_copy` (inherits the primary's current
  volume — apply `ma_sound_get_volume(primary)` to the copy explicitly; do not assume
  init_copy copies volume), starts it from frame 0, registers it in the voice table. Runs the
  LAZY REAP first (see §2). Returns false when not initialized / id unknown / 0 / init_copy
  fails. No per-call volume in this slice.
- `set_volume(id, v)`: unchanged validation; now applies to the primary AND all live copies
  (least surprise), and future copies inherit it.
- `stop_all()`: now stops copies too. `is_playing`/`voice_count`: ended-but-unreaped copies
  count as NOT playing (they read `ma_sound_is_playing`, which is what GusWorld's tests assert).
- Hardening (house rules): `fade_in_s`/`fade_out_s` rejected (return false, state untouched)
  when `!std::isfinite || < 0`; accepted values clamped to `[0, 3600]` s. **Validate the float
  BEFORE the ms cast** (`float → ma_uint64` overflow is the exact bug class from the
  audit-domino memory). All ops before init / after shutdown return false/0, never crash.

Threading contract (make explicit in audio.hpp, it was implicit until now): the `Audio` API is
single-threaded — all calls from one thread (miniaudio's own getters are thread-safe against the
mixer thread; OUR voice table is not, and gains no lock in this slice).

### 2. Lifecycle / RAII with polyphony — THE risk

Data model in `Impl`:

```cpp
std::unordered_map<SoundId, std::unique_ptr<ma_sound>> sounds;              // primaries (unchanged)
std::unordered_map<SoundId, std::vector<std::unique_ptr<ma_sound>>> voices; // live oneshot copies
```

`unique_ptr<ma_sound>` per copy for the same stable-address reason already documented for
`sounds` (ma_sound is self-referential; vector growth must not move it).

- **Reap is lazy and single-threaded by construction**: it runs ONLY inside `play_oneshot()`
  (walk that id's vector, `ma_sound_uninit` + erase entries where `ma_sound_at_end()` is true).
  No timer, no background thread → it CANNOT race `shutdown()` because both run on the caller's
  thread. That is the whole design: determinism by structure, not by locking.
- **Voice cap + stealing**: max **32 live copies per id**. If still at cap after reaping, steal
  (uninit + erase) the OLDEST copy and proceed — standard audio voice-stealing; the newest hit
  is always audible. Deterministic and testable (spam 40 oneshots of a long WAV →
  `voice_count(id) <= 32`, and `play_oneshot` never returns false due to the cap).
- **`shutdown()` ordering (extends the existing non-negotiable block)**: uninit ALL copies
  (`voices`) FIRST, then all primaries (`sounds`), then `ma_engine_uninit`, then resource
  manager, then context. Copies share refcounted data-buffer nodes in the resource manager with
  their primaries — copies-before-primaries is the hygienic order. The existing "ASan blind
  spot" comment MUST be preserved and extended to mention the voice table.
- Re-init after shutdown: `voices` cleared; old ids stay invalid forever (unchanged monotonic
  `next_id` contract).

### 3. Tests (null backend — what is provable headless)

The null backend DOES advance time in real time (miniaudio's null device runs a timer-paced
device thread), so short WAVs (~12.5 ms at 8 kHz, the existing `build_minimal_wav(100)`) end
quickly and time-based assertions work with generous poll-with-timeout loops (5 s cap — never a
bare sleep+assert; poll the condition).

Deterministic (no clock): `is_playing` true right after `play`, false right after `stop(id, 0)`;
all new ops false on unknown/0/uninitialized; `voice_count` == N right after N oneshots of a
LONG wav; cap/steal ≤ 32; shutdown with live loop + live fading sound + live copies → clean
(the ASan/LSan run is the proof of no leak/UAF); re-init works after.

Time-based (poll ≤ 5 s): loop proof — non-loop twin ends (`is_playing` → false) while
`play(id, /*loop=*/true)` twin is STILL playing after 300 ms and after the twin ended; fade-out
proof — after `stop(id, 0.05f)`, `is_playing` is still true immediately (fading = the
observable) then becomes false; the play-after-fade-stop regression (§1) — `play` again after a
fade-stop and assert it keeps playing past the old fade window; reap proof — 3 oneshots of the
short wav, poll `voice_count` → 0, then one more `play_oneshot` (triggers reap) and ASan/LSan at
shutdown proves the reaped memory was returned.

Not provable headless (explicitly): fade-in AUDIBILITY and loop GAPLESSNESS (no PCM tap on the
null device). Covered by: code review of the exact miniaudio calls + a manual smoke on a real
host (GusWorld gets it for free in their migration; note it in the relay).

Files: extend `audio_playback_sanity.cpp` (is_playing basics + hardening of new params), extend
`audio_lifecycle_sanity.cpp` (shutdown with live loop/fade/copies + re-init), NEW
`audio_polyphony_sanity.cpp` (voice_count, cap/steal, reap, oneshot semantics), NEW
`audio_loop_fade_sanity.cpp` (loop twin proof, fade-out observable, play-after-fade-stop
regression). Extend `audio_hostile_sanity.cpp` (NaN/Inf/negative/1e30 fades, oneshot on hostile
ids, oneshot spam).

### 4. MSVC / win-atoms (this slice TOUCHES Windows CI)

`audio.cpp` + all `audio_*.cpp` compile in `tests/win-atoms/` (MSVC) and the workflow
`.github/workflows/windows-atoms.yml` runs on the PR. Requirements: add the 2 NEW test files to
BOTH `glintfx/tests/CMakeLists.txt` AND `glintfx/tests/win-atoms/CMakeLists.txt` (the win harness
lists tests explicitly — forgetting it silently skips Windows coverage). Path discipline: always
`fs::path::string().c_str()` (narrow) into `load_sound` — never `path.c_str()` (that is
`wchar_t*` on Windows; the v0.15.0 gotcha). No designated-initializer edge cases, no VLAs,
`ma_uint64` conversions explicit. `/W4`-clean like the existing atoms.

### 5. Docs / release

- `docs/audio.md`: the 4 features + semantics table; the **ODR migration warning** (glintfx
  ships its own `MINIAUDIO_IMPLEMENTATION` TU — a consumer migrating from direct miniaudio MUST
  delete its own implementation TU or the link fails/ODR-breaks; this is inherent, not a bug);
  streaming = known limitation (music fully decoded in RAM).
- `audio.hpp`: rewrite the POLYPHONY paragraph (no longer deferred), add the threading contract.
- `CHANGELOG.md` + version bump to **0.18.0** in glintfx's CMake (`version()` must follow —
  the v0.2.5 stuck-version bug class).
- INBOX (TODO.md): `AUDIO-STREAM` (MA_SOUND_FLAG_STREAM for long music, non-blocking per
  consumer) — record only.
- Relay block for the leader to paste to GusWorld at the end (API mapping + ODR warning +
  "smoke fade/loop audibility on your real device").

### 6. Cost / order / headcount (3 agents, orchestrator does fan-out)

Relative cost: polyphony ≈ 60 % of the slice (new voice table + reap + steal + shutdown +
tests); fade ≈ 25 % (reset traps); loop + is_playing ≈ 15 %. Implementation order inside the
single wave: is_playing → loop → fade → polyphony (each step lands with its tests green before
the next). ONE wave, one PR — GusWorld needs all four to migrate, partial delivery unblocks
nothing.

1. **AUDIO-F2.1 — implementer** (`backend-engineer`): audio.hpp/audio.cpp + the 5 test files +
   both CMakeLists. Build in `/var/tmp` (`TMPDIR=/var/tmp`), 1 heavy build at a time. NO push.
2. **AUDIO-F2.2 — adversarial review** (`qa-engineer`, EXECUTES): ASan/LSan build, mutation of
   reap/shutdown ordering, UAF/leak attempts with live loop+fade+copies at shutdown, hostile
   floats, oneshot spam, win-atoms listing check, spot-check of the miniaudio calls against the
   vendored definitions (incl. the misnamed-declaration trap). NO push.
3. **AUDIO-F2.3 — docs/release** (`technical-writer`): §5 items. NO push.

Orchestrator: re-verifies build + claims, runs the local static gate (clang-tidy, cppcheck,
gitleaks), lets windows-atoms run on the PR, tags v0.18.0 per autonomous-mode rules (CI red
blocks).

Blocking questions: **none**. Deferred-by-decision (recorded for retroactive confirmation):
voice cap = 32/id with oldest-steal; no per-call volume on `play_oneshot`; copies never loop;
`set_volume` applies to live copies.

---

## Parte B — PT

### 0. Por quê / escopo

O consumidor GusWorld está migrando do miniaudio direto pro `glintfx::Audio` (A3-AUDIO, v0.14).
O contrato RAII/shutdown determinístico é o que eles queriam, mas 4 comportamentos que eles já
têm em produção faltam — trocar hoje REGREDIRIA o áudio do jogo. Os 4 mapeiam pra chamadas
nativas do miniaudio (verificadas no header vendorizado):

1. **Loop gapless** (crítico) — `ma_sound_set_looping` (decl. ~11462).
2. **Fade in/out** — `ma_sound_set_fade_in_milliseconds` (~11449) +
   `ma_sound_stop_with_fade_in_milliseconds` (~11403). ⚠ O nome do parâmetro na DECLARAÇÃO
   está errado (`fadeLengthInFrames`); a definição (~78811) recebe MILISSEGUNDOS e converte pelo
   sample rate. Confie na definição.
3. **Polifonia de SFX** — `ma_sound_init_copy` (~11393) por play sobreposto + reap preguiçoso
   via `ma_sound_at_end` (~11464). Hoje declarada-adiada no audio.hpp.
4. **`is_playing(SoundId)`** — `ma_sound_is_playing` (~11459); 3+ testes headless do GusWorld
   dependem.

Fora de escopo (registrado, não bloqueia): streaming (`MA_SOUND_FLAG_STREAM`) → INBOX; grupos/
buses (o consumidor emula com volume por-som × master); OGG.

### 1. Superfície de API (decidida)

Retrocompatível em nível de fonte — `play(id)` / `stop(id)` existentes continuam compilando com
a MESMA semântica (uma instância "primária" por SoundId, reinício do frame 0).

```cpp
// Estendidos (parâmetros com default — source-compatible):
bool play(SoundId id, bool loop = false, float fade_in_s = 0.0f);
bool stop(SoundId id, float fade_out_s = 0.0f);

// Novos:
bool set_looping(SoundId id, bool loop);          // toggle ao vivo na instância primária
bool play_oneshot(SoundId id);                    // cópia fire-and-forget sobreposta, nunca loopa
bool is_playing(SoundId id) const;                // true se a primária OU qualquer cópia viva toca
std::uint32_t voice_count(SoundId id) const;      // nº de instâncias tocando (primária + cópias)
```

Semântica (o contrato, a doc-comentar bilíngue no audio.hpp):

- `play(id, loop, fade_in_s)`: age SÓ na primária. Sempre seek pro frame 0 (contrato de
  reinício inalterado). DEVE chamar `ma_sound_reset_stop_time` + `ma_sound_reset_fade` (ou o
  reset combinado, se existir no header vendorizado) ANTES do seek+start — senão um
  `stop(id, fade)` agendado antes mata silenciosamente a reprodução nova (o próprio comentário
  do miniaudio avisa; teste de regressão obrigatório, ver §3). Fade-in:
  `ma_sound_set_fade_in_milliseconds(snd, 0.0f, FIM, ms)` onde `FIM` é o volume ATUAL do som
  (`ma_sound_get_volume`), pra `set_volume(id, 0.5f)` sobreviver ao fade-in (implementer
  verifica: a semântica do sentinela `-1` difere entre begin/end — não chutar, ler a
  implementação vendorizada).
- `stop(id, fade_out_s)`: `0` → `ma_sound_stop` imediato (comportamento de hoje). `> 0` →
  `ma_sound_stop_with_fade_in_milliseconds` na primária E em toda cópia viva daquele id.
  Durante o fade-out, `is_playing` continua true (documentar — é ESSE o observável).
- `set_looping(id, loop)`: só primária, aplica ao vivo. Cópia NUNCA loopa (uma cópia órfã em
  loop jamais seria reapada — remoção deliberada de footgun, documentada).
- `play_oneshot(id)`: cria cópia via `ma_sound_init_copy` (herda o volume atual da primária —
  aplicar `ma_sound_get_volume(primária)` na cópia explicitamente; não assumir que init_copy
  copia volume), inicia do frame 0, registra na tabela de vozes. Roda o REAP PREGUIÇOSO antes
  (§2). Retorna false se não inicializado / id desconhecido / 0 / falha do init_copy. Sem
  volume por-chamada nesta fatia.
- `set_volume(id, v)`: validação inalterada; agora aplica na primária E em toda cópia viva
  (menor surpresa), e cópias futuras herdam.
- `stop_all()`: agora para as cópias também. `is_playing`/`voice_count`: cópia terminada-mas-
  não-reapada conta como NÃO tocando (leem `ma_sound_is_playing` — é o que os testes do
  GusWorld assertam).
- Hardening (regra da casa): `fade_in_s`/`fade_out_s` rejeitados (false, estado intocado) se
  `!std::isfinite || < 0`; valor aceito clampeado em `[0, 3600]` s. **Validar o float ANTES do
  cast pra ms** (`float → ma_uint64` estourando é exatamente a classe de bug da memória de
  auditoria-dominó). Toda operação pré-init / pós-shutdown retorna false/0, nunca crasha.

Contrato de threading (explicitar no audio.hpp, era implícito): a API do `Audio` é
single-thread — todas as chamadas da mesma thread (os getters do miniaudio são seguros contra a
thread do mixer; a NOSSA tabela de vozes não é, e não ganha lock nesta fatia).

### 2. Ciclo de vida / RAII com polifonia — O risco

Modelo de dados no `Impl`:

```cpp
std::unordered_map<SoundId, std::unique_ptr<ma_sound>> sounds;              // primárias (inalterado)
std::unordered_map<SoundId, std::vector<std::unique_ptr<ma_sound>>> voices; // cópias oneshot vivas
```

`unique_ptr<ma_sound>` por cópia pela mesma razão de endereço-estável já documentada pro
`sounds` (ma_sound é auto-referencial; crescimento do vector não pode movê-lo).

- **Reap preguiçoso e single-thread por construção**: roda SÓ dentro do `play_oneshot()`
  (varre o vector daquele id, `ma_sound_uninit` + erase onde `ma_sound_at_end()` é true). Sem
  timer, sem thread de fundo → NÃO PODE correr com o `shutdown()` porque ambos rodam na thread
  chamadora. Esse é o design inteiro: determinismo por estrutura, não por lock.
- **Cap de vozes + stealing**: máx **32 cópias vivas por id**. Se ainda no cap após o reap,
  rouba (uninit + erase) a cópia MAIS ANTIGA e prossegue — voice-stealing padrão de áudio; o
  hit mais novo sempre soa. Determinístico e testável (spam de 40 oneshots de um WAV longo →
  `voice_count(id) <= 32`, e `play_oneshot` nunca retorna false por causa do cap).
- **Ordem do `shutdown()` (estende o bloco inegociável existente)**: uninit de TODAS as cópias
  (`voices`) PRIMEIRO, depois todas as primárias (`sounds`), depois `ma_engine_uninit`, depois
  resource manager, depois contexto. Cópias compartilham data-buffer nodes refcontados no
  resource manager com as primárias — cópias-antes-das-primárias é a ordem higiênica. O
  comentário "ponto cego do ASan" existente DEVE ser preservado e estendido pra citar a tabela
  de vozes.
- Re-init pós-shutdown: `voices` limpa; ids antigos seguem inválidos pra sempre (contrato
  monotônico do `next_id` inalterado).

### 3. Testes (null backend — o que é provável headless)

O backend null AVANÇA tempo em tempo real (o null device do miniaudio roda uma thread de device
cadenciada por timer), então WAVs curtos (~12,5 ms a 8 kHz, o `build_minimal_wav(100)`
existente) terminam rápido e asserções temporais funcionam com laços de poll-com-timeout
generoso (teto 5 s — nunca sleep+assert seco; fazer poll da condição).

Determinísticos (sem relógio): `is_playing` true logo após `play`, false logo após
`stop(id, 0)`; todas as ops novas false com id desconhecido/0/não-inicializado; `voice_count`
== N logo após N oneshots de um WAV LONGO; cap/steal ≤ 32; shutdown com loop vivo + som em
fade + cópias vivas → limpo (a rodada ASan/LSan é a prova de zero leak/UAF); re-init funciona
depois.

Temporais (poll ≤ 5 s): prova de loop — o gêmeo sem loop termina (`is_playing` → false)
enquanto o gêmeo `play(id, /*loop=*/true)` SEGUE tocando após 300 ms e após o gêmeo morrer;
prova de fade-out — após `stop(id, 0.05f)`, `is_playing` ainda true imediatamente (o fade É o
observável) e depois vira false; a regressão play-após-fade-stop (§1) — `play` de novo após um
fade-stop e assertar que segue tocando além da janela do fade antigo; prova de reap — 3
oneshots do WAV curto, poll `voice_count` → 0, mais um `play_oneshot` (dispara o reap) e o
ASan/LSan no shutdown prova que a memória reapada voltou.

Não-provável headless (explícito): AUDIBILIDADE do fade-in e o GAPLESS do loop (sem tap de PCM
no null device). Coberto por: review de código das chamadas exatas + smoke manual em host real
(o GusWorld ganha isso de graça na migração; anotar no relay).

Arquivos: estender `audio_playback_sanity.cpp` (básico de is_playing + hardening dos params
novos), estender `audio_lifecycle_sanity.cpp` (shutdown com loop/fade/cópias vivos + re-init),
NOVO `audio_polyphony_sanity.cpp` (voice_count, cap/steal, reap, semântica de oneshot), NOVO
`audio_loop_fade_sanity.cpp` (prova dos gêmeos, observável do fade-out, regressão
play-após-fade-stop). Estender `audio_hostile_sanity.cpp` (fades NaN/Inf/negativo/1e30, oneshot
em ids hostis, spam de oneshot).

### 4. MSVC / win-atoms (esta fatia TOCA a CI Windows)

`audio.cpp` + todos os `audio_*.cpp` compilam em `tests/win-atoms/` (MSVC) e o workflow
`.github/workflows/windows-atoms.yml` roda no PR. Requisitos: adicionar os 2 testes NOVOS nos
DOIS CMakeLists — `glintfx/tests/CMakeLists.txt` E `glintfx/tests/win-atoms/CMakeLists.txt` (o
harness win lista testes explicitamente — esquecer pula silenciosamente a cobertura Windows).
Disciplina de path: sempre `fs::path::string().c_str()` (narrow) pro `load_sound` — nunca
`path.c_str()` (é `wchar_t*` no Windows; o gotcha da v0.15.0). Sem VLA, conversões `ma_uint64`
explícitas, limpo em `/W4` como os átomos existentes.

### 5. Docs / release

- `docs/audio.md`: as 4 features + tabela de semântica; o **aviso de migração ODR** (a glintfx
  já embarca a TU `MINIAUDIO_IMPLEMENTATION` — consumidor migrando do miniaudio direto DEVE
  apagar a própria TU de implementação, senão o link falha/quebra ODR; é inerente, não bug);
  streaming = limitação conhecida (música decodada inteira em RAM).
- `audio.hpp`: reescrever o parágrafo POLIFONIA (não é mais adiada) + contrato de threading.
- `CHANGELOG.md` + bump pra **0.18.0** no CMake do glintfx (`version()` tem que acompanhar — a
  classe de bug da versão travada da v0.2.5).
- INBOX (TODO.md): `AUDIO-STREAM` (MA_SOUND_FLAG_STREAM pra música longa, não-bloqueante
  segundo o consumidor) — só registrar.
- Bloco de relay pro líder colar no GusWorld ao final (mapa da API + aviso ODR + "smoke de
  audibilidade de fade/loop no device real de vocês").

### 6. Custo / ordem / headcount (3 agentes, fan-out pelo orquestrador)

Custo relativo: polifonia ≈ 60 % da fatia (tabela de vozes + reap + steal + shutdown + testes);
fade ≈ 25 % (armadilhas de reset); loop + is_playing ≈ 15 %. Ordem interna da onda única:
is_playing → loop → fade → polifonia (cada passo aterrissa com os testes verdes antes do
próximo). UMA onda, um PR — o GusWorld precisa das 4 pra migrar; entrega parcial não desbloqueia
nada.

1. **AUDIO-F2.1 — implementer** (`backend-engineer`): audio.hpp/audio.cpp + os 5 arquivos de
   teste + os dois CMakeLists. Build em `/var/tmp` (`TMPDIR=/var/tmp`), 1 build pesado por vez.
   SEM push.
2. **AUDIO-F2.2 — review adversarial** (`qa-engineer`, EXECUTA): build ASan/LSan, mutação da
   ordem reap/shutdown, tentativas de UAF/leak com loop+fade+cópias vivos no shutdown, floats
   hostis, spam de oneshot, checagem da listagem no win-atoms, spot-check das chamadas miniaudio
   contra as definições vendorizadas (incl. a armadilha da declaração com nome errado). SEM push.
3. **AUDIO-F2.3 — docs/release** (`technical-writer`): itens do §5. SEM push.

Orquestrador: re-verifica build + claims, roda o gate estático local (clang-tidy, cppcheck,
gitleaks), deixa o windows-atoms rodar no PR, taggeia v0.18.0 pelas regras do modo autônomo (CI
vermelho bloqueia).

Perguntas bloqueantes: **nenhuma**. Decisões autônomas a confirmar retroativamente: cap de 32
vozes/id com steal do mais antigo; sem volume por-chamada no `play_oneshot`; cópia nunca loopa;
`set_volume` aplica nas cópias vivas.
