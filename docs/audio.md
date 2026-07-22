# Audio (A3-AUDIO reference and how-to)

> **EN:** `glintfx::Audio` API reference, lifecycle contract, supported formats, and the headless null backend used by this module's own tests. Diátaxis type: **how-to / reference**. Audience: a developer wiring sound effects/music into a glintfx-based game or app.
> **PT:** Referência da API `glintfx::Audio`, contrato de ciclo de vida, formatos suportados, e o backend null headless usado pela própria suíte de testes deste módulo. Tipo Diátaxis: **how-to / reference**. Audiência: dev conectando efeito sonoro/música num jogo ou app baseado em glintfx.

`glintfx::Audio` (`glintfx/include/glintfx/audio.hpp`) is the sound-effect and music playback
facade for the `GLINTFX_MODULE_AUDIO` atom (ADR-0015 (b) framework-2D architecture), built on
the vendored [miniaudio](https://github.com/mackron/miniaudio) library (see
`glintfx/third_party/miniaudio/README.md` for the pinned version). It depends on `core` ONLY --
no GL, no window, no RmlUi type -- so it can be exercised by unit tests with no Xvfb, same
independence class as the `i18n` module.

---

## English

### Lifecycle (read this before anything else)

```cpp
glintfx::Audio audio;                 // touches nothing yet
glintfx::AudioConfig cfg;
audio.init(cfg);                      // opens the real audio device
glintfx::Audio::SoundId id = audio.load_sound("assets/hit.wav");
audio.play(id);
// ... later, on shutdown ...
audio.shutdown();                     // or just let `audio` go out of scope
```

- `Audio()` does not touch any device or thread -- construction is always cheap.
- `init(AudioConfig)` opens the miniaudio engine. Returns `false` on failure, or if already
  initialized (call `shutdown()` first to re-init).
- `shutdown()` is **idempotent** and **deterministic**: every loaded sound is torn down
  *before* the engine itself. This ordering is the entire point of this module -- a real
  consumer (GusWorld) hit both a use-after-free and an ALSA handle leak from getting this order
  wrong by hand; `Audio` makes the correct order the only order.
- The destructor calls `shutdown()` -- an `Audio` going out of scope with sounds still loaded
  and playing is safe.
- **Every operation before a successful `init()`, or after `shutdown()`, returns `false`/`0`.**
  Never a crash, never a use-after-free. This includes calling an operation with a `SoundId`
  from a *previous* `init()`/`shutdown()` cycle -- ids are monotonic and never reused for the
  lifetime of one `Audio` object, so a stale id simply fails the lookup, safely.
- Re-`init()` after a clean `shutdown()` is fully supported (a level transition in a real game
  is exactly this cycle).
- **The sounds-before-engine order is an ASan blind spot (review adversarial finding, not a bug
  today):** `shutdown()`'s ordering is correct as shipped, but flipping it would NOT be caught
  by AddressSanitizer. `ma_engine_uninit()` never `free()`s the `ma_engine` itself (it is a
  plain member owned by `Audio`, not a separate heap allocation miniaudio owns) -- it only tears
  down what is *inside* it. A hypothetical future refactor that ran `ma_engine_uninit()` before
  the per-sound `ma_sound_uninit()` calls would touch memory that is still live and
  un-poisoned, so no sanitizer flags it; the breakage would be silent, undefined-behaviour-class
  reads of a half-torn-down engine. This is documented at the exact call site in
  `glintfx/src/audio.cpp`'s `shutdown()` -- read that comment before touching the ordering.
  **The same blind spot now also covers the voice table (`AUDIO-F2`):** every oneshot copy
  (`Impl::voices`) must be uninitialized before the primaries, for the same refcounted
  data-buffer-node reason.

### API reference

| Method | Returns | Notes |
| :--- | :--- | :--- |
| `init(AudioConfig cfg = {})` | `bool` | Opens the engine. `false` if already initialized or on device failure. |
| `shutdown()` | `void` | Idempotent; tears down sounds, then the engine. |
| `is_initialized()` | `bool` | |
| `load_sound(const char* path)` | `SoundId` (`0` = failure) | Synchronous full decode (see "Formats" below). `nullptr`/missing/corrupt file -> `0`, never crashes. |
| `play(SoundId id, bool loop = false, float fade_in_s = 0.0f)` | `bool` | Acts on the PRIMARY instance only; always restarts from frame 0. `loop` sets the looping state before starting. `fade_in_s > 0` ramps in from silence up to the sound's current volume, and resets any previously scheduled `stop(id, fade_out_s)` first (a fresh `play()` always wins over a stale scheduled stop). `fade_in_s` hardened like every duration below. `false` if `id` unknown/0 or `fade_in_s` rejected. |
| `stop(SoundId id, float fade_out_s = 0.0f)` | `bool` | Stops the PRIMARY instance AND every live oneshot copy; does not unload the sound. `fade_out_s == 0` (default) stops immediately. `> 0` schedules a fade-out instead -- **`is_playing(id)` stays `true` while it fades** (that is the deliberate observable). `false` if `id` unknown/0 or `fade_out_s` rejected. |
| `set_looping(SoundId id, bool loop)` | `bool` | Live toggle on the PRIMARY instance only, no restart needed. Copies never loop (see "Loop, fade, and polyphony" below). `false` if `id` unknown/0. |
| `play_oneshot(SoundId id)` | `bool` | Fire-and-forget overlapping copy of `id` (see "Loop, fade, and polyphony" below). Never loops, no per-call volume in this slice. `false` if `id` unknown/0 or the copy fails to start (never due to the voice cap -- the oldest copy is stolen instead). |
| `is_playing(SoundId id)` | `bool` (`const`) | `true` if the PRIMARY instance OR any live oneshot copy of `id` is playing. |
| `voice_count(SoundId id)` | `std::uint32_t` (`const`) | Number of instances of `id` (primary + copies) CURRENTLY PLAYING; an ended-but-unreaped copy does not count. |
| `set_volume(SoundId id, float v)` | `bool` | Rejects non-finite/negative `v` (keeps the previous volume); clamps an accepted value to `[0, 1]`. Applies to the PRIMARY AND every currently-live copy of `id`; a copy created afterwards inherits the primary's volume at *its own* creation time, not retroactively. |
| `set_master_volume(float v)` | `void` | Same validation as `set_volume`, engine-wide. |
| `stop_all()` | `void` | Stops every loaded sound -- every primary AND every live oneshot copy; does not unload any of them. |

### Threading contract

The `Audio` API is **single-threaded**: every call (`play`/`stop`/`play_oneshot`/`is_playing`/...)
must come from one thread. miniaudio's own getters (`ma_sound_is_playing`, `ma_sound_at_end`, ...)
are safe to call concurrently with the internal mixer thread by miniaudio's own design, but this
module's internal voice table is a plain `std::unordered_map` with no lock in this slice -- it is
not safe to call from two application threads at once.

### Formats

WAV is the mandatory baseline. **FLAC and MP3 are also enabled** -- they come for free from
miniaudio's built-in `dr_flac`/`dr_mp3` decoders, no extra vendoring needed. **OGG Vorbis is
NOT supported** in this slice: miniaudio needs an external `stb_vorbis` dependency to decode it,
which this slice does not vendor. If OGG support becomes a real need, vendoring `stb_vorbis.c`
alongside the existing `stb_image.h` is the natural next slice (tracked as a future item, not a
silent gap).

### Real-time safety

`load_sound()` performs a **full, synchronous decode at call time** (`MA_SOUND_FLAG_DECODE`).
This means the audio device's callback thread only ever reads already-decoded PCM from memory --
this module installs no callback of its own, so nothing glintfx-authored runs on the audio
thread. **This is also this slice's known limitation:** there is no streaming/async decode --
a large music track is decoded entirely into memory on `load_sound()`, same as a sound effect.
Streaming (`MA_SOUND_FLAG_STREAM`, for long music without the full-RAM cost) is tracked as a
future item (`AUDIO-STREAM`, see `TODO.md` INBOX), not a silent gap.

### Loop, fade, and polyphony (`AUDIO-F2`)

Four behaviours a real consumer (GusWorld) already had in production before migrating to
`glintfx::Audio` -- all four map to native miniaudio calls, and all four are backwards compatible
at the source level: existing `play(id)`/`stop(id)` calls keep compiling with their exact
original semantics (a single "primary" instance per `SoundId`, restart from frame 0).

- **Gapless loop:** `play(id, /*loop=*/true)` or `set_looping(id, true)` at any time (live toggle,
  no restart). Applies to the PRIMARY instance only.
- **Fade in/out:** `play(id, loop, fade_in_s)` ramps in from silence up to the sound's *current*
  volume (so a prior `set_volume()` survives the fade); `stop(id, fade_out_s)` ramps out instead
  of stopping immediately. `fade_in_s`/`fade_out_s` are hardened the same way as every duration in
  this module: rejected (`false`, state untouched) when `!std::isfinite(v) || v < 0`; an accepted
  value is clamped to `[0, 3600]` s. **While a `stop(id, fade_out_s)` fade is running,
  `is_playing(id)` keeps returning `true`** -- that is the deliberate, documented observable (the
  sound is still audible, just ramping down). A `play()` issued during a scheduled fade-out always
  wins (it resets the pending stop before restarting).
- **SFX polyphony (`play_oneshot`):** each `SoundId` has ONE "primary" instance (the one
  `play()`/`stop()`/`set_looping()` act on) plus a pool of "oneshot copies" created by
  `play_oneshot()` for overlapping playback of the *same* effect (e.g. rapid-fire gunshots).
  Copies are fire-and-forget: they never loop (a looping orphan copy could never be reaped by
  construction -- a deliberate footgun removal), they inherit the primary's volume at the moment
  they are created (not live-linked to it afterwards), and they are lazily reaped only from
  inside `play_oneshot()` itself once a copy has finished playing -- no timer, no background
  thread, so the reap cannot race `shutdown()` (both run on the caller's single thread). A hard
  cap of **32 live copies per `SoundId`** applies; once at the cap (after reaping), the OLDEST
  copy is stolen so the newest hit is always audible -- standard, deterministic audio
  voice-stealing, never a rejected `play_oneshot()` call due to the cap.
- **`is_playing`/`voice_count`:** both consider the primary AND every live copy. An
  ended-but-not-yet-reaped copy counts as NOT playing (both read `ma_sound_is_playing()`).

**Not provable headless, by construction:** fade-in AUDIBILITY and loop GAPLESSNESS (the headless
null backend advances a timer but has no PCM tap). Covered by code review of the exact miniaudio
calls plus a manual smoke test on a real audio device -- see the migration relay note below if
you are the GusWorld consumer.

### Migrating from direct miniaudio: the ODR warning

`glintfx` ships its own `MINIAUDIO_IMPLEMENTATION` translation unit (`glintfx/src/audio.cpp`
compiles `miniaudio.h`'s implementation once, inside the `glintfx_audio` object library). **If
your project already used miniaudio directly and defined `MINIAUDIO_IMPLEMENTATION` in its own
translation unit, you MUST delete that TU** when you switch to `GLINTFX_MODULE_AUDIO=ON` --
otherwise the link fails (or, worse, silently picks one definition) because of duplicate `ma_*`
symbols across the two implementation TUs. This is inherent to how miniaudio's single-header
model works, not a glintfx bug: two `MINIAUDIO_IMPLEMENTATION` translation units in one binary
is always an ODR violation, regardless of who wrote either of them.

### Null backend (headless / CI)

```cpp
glintfx::AudioConfig cfg;
cfg.null_backend = true;   // no ALSA/PulseAudio/Xvfb dependency
audio.init(cfg);
```

`AudioConfig::null_backend = true` opens a private `ma_context` pinned to miniaudio's null
backend instead of probing the host's real audio devices. This is exactly what this module's own
test suite (`glintfx/tests/audio_*_sanity.cpp`) uses -- they run in any CI container, headless,
with no sound hardware and no Xvfb.

### Volume semantics

`set_volume`/`set_master_volume` reject a non-finite (`NaN`/`Inf`) or negative value and **keep
the previous value unchanged** (the same "reject, keep previous" idiom `UiLayer::set_dp_ratio`
uses). An accepted value is clamped to `[0, 1]` -- there is no amplification past unity in this
slice; relaxing that clamp later, if a real use case needs it, is a one-line change.

### Related

- `glintfx/include/glintfx/audio.hpp` -- full doc-commented API contract (source of truth).
- `glintfx/third_party/miniaudio/README.md` -- vendored library provenance/pin, and the one
  disclosed one-line patch fixing a genuine upstream heap-use-after-free found while building
  this module (a synchronous `load_sound()` failure on a missing/hostile file used to
  use-after-free inside miniaudio's own resource manager).
- `glintfx/docs/adr/0015-framework-2d-atomized-architecture.md` -- the atomized-module
  architecture this module is a slice of.

---

## Português

### Ciclo de vida (leia isto antes de qualquer coisa)

```cpp
glintfx::Audio audio;                 // não toca em nada ainda
glintfx::AudioConfig cfg;
audio.init(cfg);                      // abre o device de áudio real
glintfx::Audio::SoundId id = audio.load_sound("assets/hit.wav");
audio.play(id);
// ... depois, no desligamento ...
audio.shutdown();                     // ou simplesmente deixe `audio` sair de escopo
```

- `Audio()` não toca em nenhum device ou thread -- a construção é sempre barata.
- `init(AudioConfig)` abre o engine miniaudio. Retorna `false` em falha, ou se já
  inicializado (chame `shutdown()` primeiro pra re-inicializar).
- `shutdown()` é **idempotente** e **determinístico**: todo som carregado é destruído *antes*
  do próprio engine. Essa ordem é o ponto central deste módulo -- um consumidor real (GusWorld)
  sofreu tanto use-after-free quanto vazamento de handle ALSA por errar essa ordem à mão;
  `Audio` torna a ordem correta a única ordem possível.
- O destrutor chama `shutdown()` -- um `Audio` saindo de escopo com sons ainda carregados e
  tocando é seguro.
- **Toda operação antes de um `init()` bem-sucedido, ou depois de `shutdown()`, retorna
  `false`/`0`.** Nunca um crash, nunca um use-after-free. Isso inclui chamar uma operação com um
  `SoundId` de um ciclo *anterior* de `init()`/`shutdown()` -- ids são monotônicos e nunca
  reusados durante a vida de um objeto `Audio`, então um id velho simplesmente falha a busca,
  com segurança.
- Re-`init()` depois de um `shutdown()` limpo é totalmente suportado (uma transição de nível
  num jogo de verdade é exatamente esse ciclo).
- **A ordem sons-antes-do-engine é um ponto cego do ASan (achado do review adversarial, não é
  bug hoje):** a ordem do `shutdown()` está correta como entregue, mas invertê-la NÃO seria
  pega pelo AddressSanitizer. `ma_engine_uninit()` nunca dá `free()` no próprio `ma_engine` (ele
  é um membro simples possuído pelo `Audio`, não uma alocação de heap separada que o miniaudio
  possui) -- só desmonta o que está *dentro* dele. Um refactor futuro hipotético que rodasse
  `ma_engine_uninit()` antes das chamadas `ma_sound_uninit()` por-som tocaria memória ainda viva
  e não-envenenada, então nenhum sanitizer acusa; a quebra seria silenciosa, leituras classe
  undefined-behaviour de um engine meio-desmontado. Isto está documentado no ponto exato de
  chamada em `glintfx/src/audio.cpp`, no `shutdown()` -- leia aquele comentário antes de mexer
  na ordem. **O mesmo ponto cego agora também cobre a tabela de vozes (`AUDIO-F2`):** toda cópia
  oneshot (`Impl::voices`) precisa ser desinicializada antes das primárias, pelo mesmo motivo de
  data-buffer-node refcontado.

### Referência de API

| Método | Retorna | Notas |
| :--- | :--- | :--- |
| `init(AudioConfig cfg = {})` | `bool` | Abre o engine. `false` se já inicializado ou em falha de device. |
| `shutdown()` | `void` | Idempotente; destrói sons, depois o engine. |
| `is_initialized()` | `bool` | |
| `load_sound(const char* path)` | `SoundId` (`0` = falha) | Decode completo síncrono (ver "Formatos" abaixo). `nullptr`/arquivo ausente/corrompido -> `0`, nunca crasha. |
| `play(SoundId id, bool loop = false, float fade_in_s = 0.0f)` | `bool` | Age só na instância PRIMÁRIA; sempre reinicia do frame 0. `loop` define o estado de loop antes de iniciar. `fade_in_s > 0` faz rampa a partir do silêncio até o volume atual do som, e reseta primeiro qualquer `stop(id, fade_out_s)` agendado antes (um `play()` novo sempre vence um stop agendado obsoleto). `fade_in_s` tem o mesmo hardening de duração abaixo. `false` se `id` desconhecido/0 ou `fade_in_s` rejeitado. |
| `stop(SoundId id, float fade_out_s = 0.0f)` | `bool` | Para a instância PRIMÁRIA E toda cópia oneshot viva; não descarrega o som. `fade_out_s == 0` (padrão) para imediatamente. `> 0` agenda um fade-out -- **`is_playing(id)` continua `true` enquanto o fade roda** (esse é o observável deliberado). `false` se `id` desconhecido/0 ou `fade_out_s` rejeitado. |
| `set_looping(SoundId id, bool loop)` | `bool` | Toggle ao vivo só na instância PRIMÁRIA, sem precisar reiniciar. Cópia nunca loopa (ver "Loop, fade e polifonia" abaixo). `false` se `id` desconhecido/0. |
| `play_oneshot(SoundId id)` | `bool` | Cópia sobreposta fire-and-forget de `id` (ver "Loop, fade e polifonia" abaixo). Nunca loopa, sem volume por-chamada nesta fatia. `false` se `id` desconhecido/0 ou a cópia falha ao iniciar (nunca por causa do teto de vozes -- a cópia mais antiga é roubada em vez disso). |
| `is_playing(SoundId id)` | `bool` (`const`) | `true` se a instância PRIMÁRIA OU alguma cópia oneshot viva de `id` está tocando. |
| `voice_count(SoundId id)` | `std::uint32_t` (`const`) | Número de instâncias de `id` (primária + cópias) TOCANDO NO MOMENTO; uma cópia terminada-mas-não-reapada não conta. |
| `set_volume(SoundId id, float v)` | `bool` | Rejeita `v` não-finito/negativo (mantém o volume anterior); clampeia um valor aceito em `[0, 1]`. Aplica na PRIMÁRIA E em toda cópia viva de `id` no momento; uma cópia criada depois herda o volume da primária no MOMENTO dela própria, não retroativamente. |
| `set_master_volume(float v)` | `void` | Mesma validação do `set_volume`, no engine inteiro. |
| `stop_all()` | `void` | Para todo som carregado -- toda primária E toda cópia oneshot viva; não descarrega nenhum. |

### Contrato de threading

A API do `Audio` é **single-thread**: toda chamada (`play`/`stop`/`play_oneshot`/`is_playing`/...)
precisa vir de uma thread. Os próprios getters do miniaudio (`ma_sound_is_playing`,
`ma_sound_at_end`, ...) são seguros pra chamar concorrentemente com a thread interna do mixer por
design do próprio miniaudio, mas a tabela de vozes interna deste módulo é um `std::unordered_map`
comum, sem lock nesta fatia -- não é seguro chamar de duas threads da aplicação ao mesmo tempo.

### Formatos

WAV é a base obrigatória. **FLAC e MP3 também estão habilitados** -- vêm de graça dos decoders
embutidos `dr_flac`/`dr_mp3` do miniaudio, sem vendoring extra necessário. **OGG Vorbis NÃO é
suportado** nesta fatia: o miniaudio precisa de uma dependência externa `stb_vorbis` para
decodificá-lo, que esta fatia não vendoriza. Se suporte a OGG virar uma necessidade real,
vendorizar `stb_vorbis.c` junto do `stb_image.h` já existente é a próxima fatia natural (item
futuro rastreado, não uma lacuna silenciosa).

### Segurança real-time

`load_sound()` faz um **decode completo e síncrono no momento da chamada**
(`MA_SOUND_FLAG_DECODE`). Isso significa que a thread de callback do device de áudio só lê PCM
já decodificado da memória -- este módulo não instala nenhum callback próprio, então nada
autoral do glintfx roda na audio thread. **Esta é também a limitação conhecida desta fatia:** não
há decode em streaming/assíncrono -- uma faixa de música grande é decodificada inteira na memória
em `load_sound()`, igual a um efeito sonoro. Streaming (`MA_SOUND_FLAG_STREAM`, pra música longa
sem o custo de RAM cheia) está rastreado como item futuro (`AUDIO-STREAM`, ver INBOX do
`TODO.md`), não uma lacuna silenciosa.

### Loop, fade e polifonia (`AUDIO-F2`)

Quatro comportamentos que um consumidor real (GusWorld) já tinha em produção antes de migrar pro
`glintfx::Audio` -- os 4 mapeiam pra chamadas nativas do miniaudio, e os 4 são retrocompatíveis em
nível de fonte: as chamadas `play(id)`/`stop(id)` existentes continuam compilando com a MESMA
semântica original (uma instância "primária" por `SoundId`, reinício do frame 0).

- **Loop gapless:** `play(id, /*loop=*/true)` ou `set_looping(id, true)` a qualquer momento
  (toggle ao vivo, sem reinício). Aplica só na instância PRIMÁRIA.
- **Fade in/out:** `play(id, loop, fade_in_s)` faz rampa a partir do silêncio até o volume
  *atual* do som (pra um `set_volume()` anterior sobreviver ao fade); `stop(id, fade_out_s)` faz
  rampa de saída em vez de parar imediatamente. `fade_in_s`/`fade_out_s` têm o mesmo hardening de
  toda duração deste módulo: rejeitados (`false`, estado intocado) quando
  `!std::isfinite(v) || v < 0`; um valor aceito é clampeado em `[0, 3600]` s. **Enquanto um fade
  de `stop(id, fade_out_s)` roda, `is_playing(id)` continua retornando `true`** -- esse é o
  observável deliberado e documentado (o som ainda está audível, só decaindo). Um `play()`
  disparado durante um fade-out agendado sempre vence (reseta o stop pendente antes de reiniciar).
- **Polifonia de SFX (`play_oneshot`):** cada `SoundId` tem UMA instância "primária" (a que
  `play()`/`stop()`/`set_looping()` afetam) mais um pool de "cópias oneshot" criadas por
  `play_oneshot()` pra reprodução sobreposta do *mesmo* efeito (ex.: tiros em rajada). Cópias são
  fire-and-forget: nunca loopam (uma cópia órfã em loop jamais seria reapada por construção --
  remoção deliberada de footgun), herdam o volume da primária no momento em que são criadas (não
  ficam ligadas ao vivo depois), e são reapadas preguiçosamente só de dentro do próprio
  `play_oneshot()` quando uma cópia termina de tocar -- sem timer, sem thread de fundo, então o
  reap não pode correr com o `shutdown()` (ambos rodam na única thread chamadora). Um teto rígido
  de **32 cópias vivas por `SoundId`** se aplica; ao atingir o teto (depois do reap), a cópia MAIS
  ANTIGA é roubada pra que o hit mais novo sempre soe -- voice-stealing de áudio padrão e
  determinístico, nunca uma chamada `play_oneshot()` rejeitada por causa do teto.
- **`is_playing`/`voice_count`:** ambos consideram a primária E toda cópia viva. Uma cópia
  terminada-mas-ainda-não-reapada conta como NÃO tocando (ambos leem `ma_sound_is_playing()`).

**Não-provável headless, por construção:** AUDIBILIDADE do fade-in e o GAPLESS do loop (o backend
null headless avança um timer mas não tem tap de PCM). Coberto por review de código das chamadas
exatas do miniaudio mais um smoke manual num device de áudio real -- ver a nota de relay de
migração abaixo se você é o consumidor GusWorld.

### Migrando do miniaudio direto: o aviso ODR

A glintfx já embarca a própria unidade de tradução `MINIAUDIO_IMPLEMENTATION`
(`glintfx/src/audio.cpp` compila a implementação do `miniaudio.h` uma vez, dentro da object
library `glintfx_audio`). **Se seu projeto já usava o miniaudio direto e definia
`MINIAUDIO_IMPLEMENTATION` na própria TU, você DEVE apagar essa TU** ao migrar pra
`GLINTFX_MODULE_AUDIO=ON` -- senão o link falha (ou, pior, escolhe uma definição em silêncio) por
causa de símbolos `ma_*` duplicados entre as duas TUs de implementação. Isso é inerente a como o
modelo single-header do miniaudio funciona, não um bug da glintfx: duas TUs
`MINIAUDIO_IMPLEMENTATION` num mesmo binário sempre são uma violação de ODR, não importa quem
escreveu qual delas.

### Backend null (headless / CI)

```cpp
glintfx::AudioConfig cfg;
cfg.null_backend = true;   // sem dependência de ALSA/PulseAudio/Xvfb
audio.init(cfg);
```

`AudioConfig::null_backend = true` abre um `ma_context` privado fixado no backend null do
miniaudio, em vez de sondar os devices de áudio reais do host. É exatamente isso que a própria
suíte de testes deste módulo (`glintfx/tests/audio_*_sanity.cpp`) usa -- eles rodam em qualquer
container de CI, headless, sem hardware de som e sem Xvfb.

### Semântica de volume

`set_volume`/`set_master_volume` rejeitam um valor não-finito (`NaN`/`Inf`) ou negativo e
**mantêm o valor anterior inalterado** (mesmo idioma "rejeita, mantém o anterior" que o
`UiLayer::set_dp_ratio` usa). Um valor aceito é clampeado em `[0, 1]` -- não há amplificação
além da unidade nesta fatia; relaxar esse clamp depois, se um caso de uso real precisar, é uma
mudança de uma linha.

### Relacionados

- `glintfx/include/glintfx/audio.hpp` -- contrato de API completo com doc-comments (fonte da
  verdade).
- `glintfx/third_party/miniaudio/README.md` -- procedência/pin da biblioteca vendorizada, e o
  único patch de uma linha divulgado que corrige um heap-use-after-free genuíno do upstream
  encontrado ao construir este módulo (uma falha síncrona do `load_sound()` num arquivo
  ausente/hostil causava use-after-free dentro do próprio resource manager do miniaudio).
- `glintfx/docs/adr/0015-framework-2d-atomized-architecture.md` -- a arquitetura de módulos
  atomizados da qual este módulo é uma fatia.
