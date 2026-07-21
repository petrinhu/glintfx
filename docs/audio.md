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

### API reference

| Method | Returns | Notes |
| :--- | :--- | :--- |
| `init(AudioConfig cfg = {})` | `bool` | Opens the engine. `false` if already initialized or on device failure. |
| `shutdown()` | `void` | Idempotent; tears down sounds, then the engine. |
| `is_initialized()` | `bool` | |
| `load_sound(const char* path)` | `SoundId` (`0` = failure) | Synchronous full decode (see "Formats" below). `nullptr`/missing/corrupt file -> `0`, never crashes. |
| `play(SoundId id)` | `bool` | Always restarts from frame 0 (see "Polyphony" below). `false` if `id` unknown or `0`. |
| `stop(SoundId id)` | `bool` | Does not unload the sound. |
| `set_volume(SoundId id, float v)` | `bool` | Rejects non-finite/negative `v` (keeps the previous volume); clamps an accepted value to `[0, 1]`. |
| `set_master_volume(float v)` | `void` | Same validation as `set_volume`, engine-wide. |
| `stop_all()` | `void` | Stops every loaded sound; does not unload any of them. |

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
thread. There is no streaming/async decode in this slice; a large music track is decoded
entirely into memory on `load_sound()`, same as a sound effect.

### Polyphony (not yet supported, by design)

One live instance per `SoundId`. Calling `play()` on a sound that is already playing restarts
it from frame 0 -- it does **not** layer a second overlapping instance. Overlapping instances of
the *same* sound effect (e.g. rapid-fire gunshots) needs `ma_sound_init_copy`, which is a future
slice, not a half-implemented feature here.

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

### Referência de API

| Método | Retorna | Notas |
| :--- | :--- | :--- |
| `init(AudioConfig cfg = {})` | `bool` | Abre o engine. `false` se já inicializado ou em falha de device. |
| `shutdown()` | `void` | Idempotente; destrói sons, depois o engine. |
| `is_initialized()` | `bool` | |
| `load_sound(const char* path)` | `SoundId` (`0` = falha) | Decode completo síncrono (ver "Formatos" abaixo). `nullptr`/arquivo ausente/corrompido -> `0`, nunca crasha. |
| `play(SoundId id)` | `bool` | Sempre reinicia do frame 0 (ver "Polifonia" abaixo). `false` se `id` desconhecido ou `0`. |
| `stop(SoundId id)` | `bool` | Não descarrega o som. |
| `set_volume(SoundId id, float v)` | `bool` | Rejeita `v` não-finito/negativo (mantém o volume anterior); clampeia um valor aceito em `[0, 1]`. |
| `set_master_volume(float v)` | `void` | Mesma validação do `set_volume`, no engine inteiro. |
| `stop_all()` | `void` | Para todo som carregado; não descarrega nenhum. |

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
autoral do glintfx roda na audio thread. Não há decode em streaming/assíncrono nesta fatia; uma
faixa de música grande é decodificada inteira na memória em `load_sound()`, igual a um efeito
sonoro.

### Polifonia (ainda não suportada, por design)

Uma instância viva por `SoundId`. Chamar `play()` num som que já está tocando reinicia do frame
0 -- **não** empilha uma segunda instância sobreposta. Instâncias sobrepostas do *mesmo* efeito
sonoro (ex.: tiros em rajada) precisam de `ma_sound_init_copy`, que é uma fatia futura, não uma
feature meio-implementada aqui.

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
