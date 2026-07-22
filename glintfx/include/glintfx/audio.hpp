// SPDX-License-Identifier: MPL-2.0
// EN: A3-AUDIO (framework-2D, ADR-0015 module "audio") -- sound-effect and music playback via
//     the vendored miniaudio library. This is the WHOLE public surface of the module: no
//     miniaudio type (ma_engine/ma_sound/ma_context/...) appears here (pimpl, see `Impl` in
//     audio.cpp) -- same discipline as UiLayer/DataBinder hiding RmlUi types, and the same
//     independence class as `i18n` (ADR-0015 (b): "audio depends on core ONLY" -- no GL, no
//     window, no RmlUi type crossing this header, testable without Xvfb; unlike i18n this
//     module DOES own a hardware/OS resource, hence the RAII contract spelled out below).
//
//     LIFECYCLE / RAII (the non-negotiable contract of this module -- see
//     docs/superpowers/plans/2026-07-20-framework2d-A3-audio-e-carving-uifx.md section 1.2):
//     `Audio()` touches nothing (no device, no thread). `init(AudioConfig)` opens the miniaudio
//     engine (and, for `null_backend=true`, a private `ma_context` pinned to the null backend --
//     the headless-CI path, no ALSA/PulseAudio/Xvfb needed). `shutdown()` is idempotent and
//     DETERMINISTIC: every loaded `SoundId` is torn down (`ma_sound_uninit`) BEFORE the engine
//     itself (`ma_engine_uninit`) -- this exact ordering is what the GusWorld consumer's own
//     audio lifecycle got wrong (a live sound outliving the engine caused both a
//     use-after-free and an ALSA handle leak on every level transition); this module refuses to
//     let a consumer repeat that mistake by construction, not by convention. The destructor
//     calls `shutdown()`. Every operation performed before `init()` succeeds or after
//     `shutdown()` returns `false`/`0` -- never a crash, never a use-after-free (mirrors this
//     library's "input validation em borda" convention, e.g. `UiLayer::ok()`-gated calls,
//     `decorator_polygon.cpp`'s hostile-input discipline).
//
//     REAL-TIME SAFETY (structural, not a runtime check): `load_sound()` performs a FULL,
//     SYNCHRONOUS decode at call time (`MA_SOUND_FLAG_DECODE` in audio.cpp) -- the audio
//     device's callback thread therefore only ever reads already-decoded PCM from memory. This
//     module installs no callback of its own (miniaudio's engine drives mixing internally), so
//     nothing of OURS runs on the audio thread; there is no lock, no allocation, no exception
//     path of ours to reason about there by construction. Streaming/async decode is explicitly
//     out of scope for this slice.
//
//     FORMATS: WAV is the mandatory baseline; FLAC and MP3 come for free from miniaudio's
//     built-in decoders (dr_wav/dr_flac/dr_mp3) and are enabled. OGG is NOT free (miniaudio
//     needs an external stb_vorbis for it) -- deferred, see docs/audio.md.
//
//     HARDENING: `set_volume()`/`set_master_volume()` reject a non-finite (NaN/Inf) or negative
//     value, KEEPING THE PREVIOUS value unchanged (same idiom as `UiLayer::set_dp_ratio`), and
//     clamp any accepted value to `[0, 1]` (no amplification past unity in this slice --
//     deliberate, see the plan's open question 1). `load_sound(nullptr)` and a hostile/
//     truncated/garbage file both return `SoundId{0}` -- never abort the host (same discipline
//     as `decorator_polygon.cpp`/`i18n.cpp`'s malformed-input handling).
//
//     THREADING (AUDIO-F2, make explicit what was implicit before this slice): the `Audio` API
//     is SINGLE-THREADED -- every call (`play`/`stop`/`play_oneshot`/`is_playing`/...) must come
//     from one thread. miniaudio's own getters (`ma_sound_is_playing`, `ma_sound_at_end`, ...)
//     are safe to call concurrently with the internal mixer thread by miniaudio's own design, but
//     OUR voice table (`Impl::voices`, see audio.cpp) is a plain `std::unordered_map` with no
//     lock in this slice -- it is not safe to call from two application threads at once.
//
//     POLYPHONY (AUDIO-F2, no longer deferred): each `SoundId` has ONE "primary" instance (the
//     one `play()`/`stop()`/`set_looping()` act on -- unchanged single-instance restart-from-0
//     contract) plus a pool of "oneshot copies" created by `play_oneshot()`
//     (`ma_sound_init_copy()`) for overlapping playback of the SAME sound effect (e.g. rapid-fire
//     gunshots). Copies are fire-and-forget: they never loop (a looping orphan copy could never
//     be reaped by construction -- deliberate footgun removal), inherit the primary's volume at
//     the moment they are created (NOT live-linked to it afterwards -- see `set_volume()`, which
//     DOES also push to every currently-live copy), and are lazily reaped (`ma_sound_uninit()` +
//     drop) only from inside `play_oneshot()` itself once `ma_sound_at_end()` is true -- no timer,
//     no background thread, so the reap cannot race `shutdown()` (both run on the caller's single
//     thread; see this contract mirrored in audio.cpp's `Impl::voices` comment). A hard cap of 32
//     live copies per `SoundId` applies; once at the cap (after reaping), the OLDEST copy is
//     stolen (uninitialized and dropped) so the newest hit is always audible -- standard,
//     deterministic audio voice-stealing, never a rejected `play_oneshot()` call due to the cap.
//
// PT: A3-AUDIO (framework-2D, módulo "audio" da ADR-0015) -- reprodução de efeito sonoro e
//     música via a biblioteca vendorizada miniaudio. Esta é a superfície pública INTEIRA do
//     módulo: nenhum tipo miniaudio (ma_engine/ma_sound/ma_context/...) aparece aqui (pimpl, ver
//     `Impl` em audio.cpp) -- mesma disciplina do UiLayer/DataBinder escondendo tipos do RmlUi, e
//     mesma classe de independência do `i18n` (ADR-0015 (b): "audio depende SÓ de core" -- zero
//     GL, zero janela, zero tipo do RmlUi cruzando este header, testável sem Xvfb; diferente do
//     i18n este módulo POSSUI um recurso de hardware/SO, daí o contrato de RAII detalhado abaixo).
//
//     CICLO DE VIDA / RAII (o contrato inegociável deste módulo -- ver
//     docs/superpowers/plans/2026-07-20-framework2d-A3-audio-e-carving-uifx.md seção 1.2):
//     `Audio()` não toca em nada (sem device, sem thread). `init(AudioConfig)` abre o engine
//     miniaudio (e, para `null_backend=true`, um `ma_context` privado fixado no backend null --
//     o caminho de CI headless, sem ALSA/PulseAudio/Xvfb necessário). `shutdown()` é idempotente
//     e DETERMINÍSTICO: todo `SoundId` carregado é destruído (`ma_sound_uninit`) ANTES do
//     próprio engine (`ma_engine_uninit`) -- é exatamente essa ordem que o ciclo de vida de
//     áudio do consumidor GusWorld errou (um som vivo sobrevivendo ao engine causava tanto
//     use-after-free quanto vazamento de handle ALSA a cada transição de nível); este módulo se
//     recusa a deixar um consumidor repetir esse erro por construção, não por convenção. O
//     destrutor chama `shutdown()`. Toda operação feita antes de `init()` ter sucesso ou depois
//     de `shutdown()` retorna `false`/`0` -- nunca um crash, nunca um use-after-free (espelha a
//     convenção "input validation em borda" desta biblioteca, ex.: chamadas gateadas por
//     `UiLayer::ok()`, a disciplina de input hostil do `decorator_polygon.cpp`).
//
//     SEGURANÇA REAL-TIME (estrutural, não uma checagem em runtime): `load_sound()` faz um
//     decode COMPLETO e SÍNCRONO no momento da chamada (`MA_SOUND_FLAG_DECODE` em audio.cpp) --
//     a thread de callback do device de áudio portanto só lê PCM já decodificado da memória.
//     Este módulo não instala nenhum callback próprio (o engine do miniaudio comanda a mixagem
//     internamente), então nada NOSSO roda na audio thread; não há lock, alocação, nem caminho
//     de exceção nosso a considerar ali, por construção. Decode em streaming/assíncrono está
//     explicitamente fora de escopo nesta fatia.
//
//     FORMATOS: WAV é a base obrigatória; FLAC e MP3 vêm de graça dos decoders embutidos do
//     miniaudio (dr_wav/dr_flac/dr_mp3) e estão habilitados. OGG NÃO vem de graça (o miniaudio
//     precisa de um stb_vorbis externo para isso) -- adiado, ver docs/audio.md.
//
//     HARDENING: `set_volume()`/`set_master_volume()` rejeitam um valor não-finito (NaN/Inf) ou
//     negativo, MANTENDO o valor anterior inalterado (mesmo idioma do `UiLayer::set_dp_ratio`),
//     e fazem clamp de todo valor aceito em `[0, 1]` (sem amplificação além da unidade nesta
//     fatia -- deliberado, ver a pergunta em aberto 1 do plano). `load_sound(nullptr)` e um
//     arquivo hostil/truncado/lixo retornam `SoundId{0}` -- nunca abortam o host (mesma
//     disciplina do tratamento de input malformado do `decorator_polygon.cpp`/`i18n.cpp`).
//
//     THREADING (AUDIO-F2, explicitando o que era implícito antes desta fatia): a API do `Audio`
//     é SINGLE-THREADED -- toda chamada (`play`/`stop`/`play_oneshot`/`is_playing`/...) precisa
//     vir de UMA thread. Os próprios getters do miniaudio (`ma_sound_is_playing`,
//     `ma_sound_at_end`, ...) são seguros pra chamar concorrentemente com a thread interna do
//     mixer por design do próprio miniaudio, mas a NOSSA tabela de vozes (`Impl::voices`, ver
//     audio.cpp) é um `std::unordered_map` comum, sem lock nesta fatia -- não é seguro chamar de
//     duas threads da aplicação ao mesmo tempo.
//
//     POLIFONIA (AUDIO-F2, não é mais adiada): cada `SoundId` tem UMA instância "primária" (a
//     que `play()`/`stop()`/`set_looping()` afetam -- contrato inalterado de reinício-do-frame-0
//     de instância única) mais um pool de "cópias oneshot" criadas por `play_oneshot()`
//     (`ma_sound_init_copy()`) pra reprodução sobreposta do MESMO efeito sonoro (ex.: tiros em
//     rajada). Cópias são fire-and-forget: nunca loopam (uma cópia órfã em loop jamais seria
//     reapada por construção -- remoção deliberada de footgun), herdam o volume da primária no
//     MOMENTO em que são criadas (NÃO ficam ligadas ao vivo depois -- ver `set_volume()`, que
//     TAMBÉM empurra pra toda cópia viva no momento), e são reapadas preguiçosamente
//     (`ma_sound_uninit()` + descarte) só de dentro do próprio `play_oneshot()` quando
//     `ma_sound_at_end()` é true -- sem timer, sem thread de fundo, então o reap não pode correr
//     com o `shutdown()` (ambos rodam na única thread chamadora; ver este contrato espelhado no
//     comentário de `Impl::voices` do audio.cpp). Um teto rígido de 32 cópias vivas por `SoundId`
//     se aplica; ao atingir o teto (depois do reap), a cópia MAIS ANTIGA é roubada (desinicializada
//     e descartada) pra que o hit mais novo sempre soe -- voice-stealing de áudio padrão e
//     determinístico, nunca uma chamada `play_oneshot()` rejeitada por causa do teto.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstdint>
#include <memory>

namespace glintfx {

// EN: Configuration for Audio::init(). Declared at namespace scope (default-argument rule, same
//     reason as UiLayerConfig -- see ui_layer.hpp).
// PT: Configuração para Audio::init(). Declarada em namespace scope (regra de argumento padrão,
//     mesmo motivo do UiLayerConfig -- ver ui_layer.hpp).
struct AudioConfig {
  // EN: true selects miniaudio's null backend (headless CI / no sound device present) --
  //     no ALSA/PulseAudio/Xvfb dependency, used by this module's own test suite. false (the
  //     default) lets miniaudio probe the host's real backends in its normal priority order.
  // PT: true seleciona o backend null do miniaudio (CI headless / sem device de som presente)
  //     -- nenhuma dependência de ALSA/PulseAudio/Xvfb, usado pela própria suíte de testes deste
  //     módulo. false (o padrão) deixa o miniaudio sondar os backends reais do host na ordem de
  //     prioridade normal dele.
  bool null_backend = false;
};

// EN: Sound-effect and music playback facade. See this file's header comment above for the full
//     lifecycle/RAII/real-time-safety/hardening contract. No third-party (miniaudio) type
//     crosses this header -- `Impl` (audio.cpp) is where every `ma_*` type actually lives.
// PT: Fachada de reprodução de efeito sonoro e música. Ver o comentário de cabeçalho deste
//     arquivo acima para o contrato completo de ciclo-de-vida/RAII/segurança-real-time/
//     hardening. Nenhum tipo de terceiro (miniaudio) cruza este header -- `Impl` (audio.cpp) é
//     onde todo tipo `ma_*` de fato vive.
class Audio {
public:
  // EN: 0 is reserved as the invalid/failure id -- every real sound gets a monotonically
  //     increasing id starting at 1, and ids are NEVER reused across the lifetime of one Audio
  //     instance (an id from a sound loaded before a shutdown()+re-init() cycle stays invalid
  //     forever after, safely: every op checks membership, not just numeric range).
  // PT: 0 é reservado como o id inválido/de falha -- todo som real recebe um id monotonicamente
  //     crescente começando em 1, e ids NUNCA são reusados durante a vida de uma instância
  //     Audio (um id de um som carregado antes de um ciclo shutdown()+re-init() fica inválido
  //     para sempre depois, com segurança: toda operação checa pertencimento, não só faixa
  //     numérica).
  using SoundId = std::uint32_t;

  // EN: Does NOT touch the device/engine -- construction is always cheap and side-effect-free;
  //     the real resource acquisition happens in init().
  // PT: NÃO toca no device/engine -- a construção é sempre barata e sem efeito colateral; a
  //     aquisição do recurso de verdade acontece em init().
  Audio();

  // EN: Deterministic: calls shutdown() (safe even if init() was never called or already shut
  //     down -- shutdown() is idempotent).
  // PT: Determinístico: chama shutdown() (seguro mesmo se init() nunca foi chamado ou já foi
  //     desligado -- shutdown() é idempotente).
  ~Audio();

  // EN: Rule-of-5, explicit (clang-analyzer/cppcoreguidelines-special-member-functions): a
  //     user-declared destructor above suppresses the implicit move members, so copy alone
  //     would leave move ambiguous/half-declared. Spelling out all 4 states the actual policy
  //     instead of leaving it implicit: copying an `Audio` makes no sense (it owns a live
  //     device/engine and a table of native sound handles -- there is no sane "duplicate this
  //     open device" semantics), and moving one out from under a live engine is unsafe by the
  //     same RAII-ordering argument as shutdown() itself (a moved-from `Audio` would need its
  //     own well-defined "is this the one that still owns the device" state, not needed by any
  //     consumer of this module today -- revisit only if a real use case asks for it).
  // PT: Regra dos 5, explícita (clang-analyzer/cppcoreguidelines-special-member-functions): um
  //     destrutor declarado pelo usuário acima suprime os membros de move implícitos, então só
  //     deletar o copy deixaria o move ambíguo/meio-declarado. Explicitar os 4 estados diz a
  //     política de verdade em vez de deixar implícito: copiar um `Audio` não faz sentido (ele
  //     é dono de um device/engine vivo e de uma tabela de handles nativos de som -- não há
  //     semântica sã de "duplicar este device aberto"), e mover um de baixo de um engine vivo é
  //     inseguro pelo mesmo argumento de ordem de RAII do próprio shutdown() (um `Audio`
  //     movido-de precisaria do próprio estado bem definido de "este ainda é dono do device",
  //     não necessário por nenhum consumidor deste módulo hoje -- revisitar só se um caso de
  //     uso real pedir).
  Audio(const Audio&)            = delete;
  Audio& operator=(const Audio&) = delete;
  Audio(Audio&&)                 = delete;
  Audio& operator=(Audio&&)      = delete;

  // EN: Opens the miniaudio engine (and, for null_backend=true, a private null-backend
  //     context). Returns false on any miniaudio failure, OR when this instance is already
  //     initialized (call shutdown() first to re-init; re-init after a clean shutdown() is
  //     fully supported and tested).
  // PT: Abre o engine miniaudio (e, para null_backend=true, um contexto de backend null
  //     privado). Retorna false em qualquer falha do miniaudio, OU quando esta instância já
  //     está inicializada (chame shutdown() primeiro para re-inicializar; re-init após um
  //     shutdown() limpo é totalmente suportado e testado).
  bool init(const AudioConfig& cfg = {});

  // EN: Idempotent, deterministic teardown: every loaded sound is uninitialized BEFORE the
  //     engine/context (see this file's header comment) -- safe to call any number of times,
  //     including on a never-initialized instance (no-op). After this call every SoundId
  //     becomes invalid and every operation below returns false/0 until the next init().
  // PT: Teardown idempotente e determinístico: todo som carregado é desinicializado ANTES do
  //     engine/contexto (ver o comentário de cabeçalho deste arquivo) -- seguro chamar quantas
  //     vezes for, inclusive numa instância nunca inicializada (no-op). Depois desta chamada
  //     todo SoundId fica inválido e toda operação abaixo retorna false/0 até o próximo init().
  void shutdown();

  // EN: true between a successful init() and the next shutdown()/destruction.
  // PT: true entre um init() bem-sucedido e o próximo shutdown()/destruição.
  bool is_initialized() const;

  // EN: Synchronous full decode of `path` (WAV/FLAC/MP3 -- see this file's header comment) into
  //     a new sound, ready for play(). Returns 0 (never a valid id) when: not initialized,
  //     `path` is nullptr, the file cannot be opened, or its content fails to decode (truncated
  //     header, corrupt data, wrong format, empty file) -- never crashes or aborts the host on
  //     hostile input.
  // PT: Decode completo e síncrono de `path` (WAV/FLAC/MP3 -- ver o comentário de cabeçalho
  //     deste arquivo) num som novo, pronto para play(). Retorna 0 (nunca um id válido) quando:
  //     não inicializado, `path` é nullptr, o arquivo não pode ser aberto, ou o conteúdo dele
  //     falha ao decodificar (header truncado, dado corrompido, formato errado, arquivo vazio)
  //     -- nunca crasha nem aborta o host com input hostil.
  SoundId load_sound(const char* path);

  // EN: Starts (or restarts from frame 0, if already playing -- see the polyphony note above)
  //     playback of the PRIMARY instance of `id`. `loop` sets the looping state before starting
  //     (equivalent to a `set_looping(id, loop)` immediately before the restart). `fade_in_s > 0`
  //     schedules a fade-in from silence up to the sound's CURRENT volume (so a prior
  //     `set_volume()` survives the fade -- see this file's header comment); any previously
  //     scheduled `stop(id, fade_out_s)` is reset FIRST, so a fresh play() always wins over a
  //     stale scheduled stop. `fade_in_s` is hardened like every duration in this module: rejected
  //     (returns false, primary left untouched) when `!std::isfinite(fade_in_s) || fade_in_s < 0`;
  //     an accepted value is clamped to `[0, 3600]` s. Returns false when not initialized, `id` is
  //     unknown/0, or `fade_in_s` was rejected.
  // PT: Inicia (ou reinicia do frame 0, se já estiver tocando -- ver a nota de polifonia acima)
  //     a reprodução da instância PRIMÁRIA de `id`. `loop` define o estado de loop antes de
  //     iniciar (equivalente a um `set_looping(id, loop)` imediatamente antes do reinício).
  //     `fade_in_s > 0` agenda um fade-in do silêncio até o volume ATUAL do som (pra um
  //     `set_volume()` anterior sobreviver ao fade -- ver o comentário de cabeçalho deste
  //     arquivo); qualquer `stop(id, fade_out_s)` agendado antes é resetado PRIMEIRO, pra um
  //     play() novo sempre vencer um stop agendado obsoleto. `fade_in_s` tem o mesmo hardening de
  //     toda duração deste módulo: rejeitado (retorna false, primária intocada) quando
  //     `!std::isfinite(fade_in_s) || fade_in_s < 0`; um valor aceito é clampeado em `[0, 3600]`
  //     s. Retorna false quando não inicializado, `id` é desconhecido/0, ou `fade_in_s` foi
  //     rejeitado.
  bool play(SoundId id, bool loop = false, float fade_in_s = 0.0f);

  // EN: Stops playback of the PRIMARY instance of `id` AND every currently-live oneshot copy of
  //     it (does not unload any of them -- play() again resumes the primary from frame 0).
  //     `fade_out_s == 0` (the default) stops immediately (this module's original behaviour).
  //     `fade_out_s > 0` schedules a fade-out instead; WHILE fading, `is_playing(id)` keeps
  //     returning true (that is the deliberate, documented observable -- the sound is still
  //     audible, just ramping down). Same hardening as `play()`'s `fade_in_s`: `fade_out_s`
  //     rejected (returns false, nothing stopped) when non-finite or negative; accepted values
  //     clamped to `[0, 3600]` s. Returns false when not initialized, `id` is unknown/0, or
  //     `fade_out_s` was rejected.
  // PT: Para a reprodução da instância PRIMÁRIA de `id` E de toda cópia oneshot viva no momento
  //     (não descarrega nenhuma -- play() de novo retoma a primária do frame 0). `fade_out_s == 0`
  //     (o padrão) para imediatamente (comportamento original deste módulo). `fade_out_s > 0`
  //     agenda um fade-out; ENQUANTO o fade roda, `is_playing(id)` continua retornando true (é
  //     ESSE o observável deliberado e documentado -- o som ainda está audível, só decaindo).
  //     Mesmo hardening do `fade_in_s` do `play()`: `fade_out_s` rejeitado (retorna false, nada
  //     parado) se não-finito ou negativo; valores aceitos clampeados em `[0, 3600]` s. Retorna
  //     false quando não inicializado, `id` é desconhecido/0, ou `fade_out_s` foi rejeitado.
  bool stop(SoundId id, float fade_out_s = 0.0f);

  // EN: Live toggle of the looping state on `id`'s PRIMARY instance only (copies never loop --
  //     see this file's header comment). Applies immediately, no restart needed. Returns false
  //     when not initialized or `id` is unknown/0.
  // PT: Toggle ao vivo do estado de loop só na instância PRIMÁRIA de `id` (cópias nunca loopam --
  //     ver o comentário de cabeçalho deste arquivo). Aplica imediatamente, sem precisar
  //     reiniciar. Retorna false quando não inicializado ou `id` é desconhecido/0.
  bool set_looping(SoundId id, bool loop);

  // EN: Fire-and-forget overlapping playback of `id`: creates a copy of the primary
  //     (`ma_sound_init_copy()`), starts it from frame 0 at the primary's CURRENT volume, and
  //     registers it in the internal voice pool for this `id` -- see this file's header comment
  //     for the full polyphony/cap/reap contract. Runs the lazy reap of finished copies of `id`
  //     first. The copy never loops and never accepts a per-call volume in this slice. Returns
  //     false when not initialized, `id` is unknown/0, or the underlying copy fails to
  //     initialize/start (never due to the voice cap -- the oldest copy is stolen instead).
  // PT: Reprodução sobreposta fire-and-forget de `id`: cria uma cópia da primária
  //     (`ma_sound_init_copy()`), inicia do frame 0 no volume ATUAL da primária, e registra no
  //     pool de vozes interno daquele `id` -- ver o comentário de cabeçalho deste arquivo pro
  //     contrato completo de polifonia/teto/reap. Roda o reap preguiçoso das cópias terminadas de
  //     `id` primeiro. A cópia nunca loopa e não aceita volume por-chamada nesta fatia. Retorna
  //     false quando não inicializado, `id` é desconhecido/0, ou a cópia falha ao
  //     inicializar/iniciar (nunca por causa do teto de vozes -- a cópia mais antiga é roubada em
  //     vez disso).
  bool play_oneshot(SoundId id);

  // EN: true when `id`'s PRIMARY instance is playing (per `ma_sound_is_playing()`, which accounts
  //     for a scheduled stop time -- so this stays true throughout a `stop(id, fade_out_s)` fade)
  //     OR any of its live oneshot copies is. false when not initialized, `id` is unknown/0, or
  //     nothing of `id`'s is currently playing.
  // PT: true quando a instância PRIMÁRIA de `id` está tocando (conforme `ma_sound_is_playing()`,
  //     que considera um stop-time agendado -- então isto continua true durante todo um fade de
  //     `stop(id, fade_out_s)`) OU alguma cópia oneshot viva dela está. false quando não
  //     inicializado, `id` é desconhecido/0, ou nada de `id` está tocando no momento.
  bool is_playing(SoundId id) const;

  // EN: Number of instances of `id` (primary + oneshot copies) CURRENTLY PLAYING. An ended-but-
  //     not-yet-reaped copy does not count (it reads `ma_sound_is_playing()`, same as
  //     `is_playing()` above). 0 when not initialized, `id` is unknown/0, or nothing is playing.
  //     Useful to observe the reap/cap/voice-stealing behaviour from a test without reaching into
  //     internals.
  // PT: Número de instâncias de `id` (primária + cópias oneshot) TOCANDO NO MOMENTO. Uma cópia
  //     terminada-mas-ainda-não-reapada não conta (lê `ma_sound_is_playing()`, igual ao
  //     `is_playing()` acima). 0 quando não inicializado, `id` é desconhecido/0, ou nada está
  //     tocando. Útil pra observar o comportamento de reap/teto/voice-stealing a partir de um
  //     teste sem alcançar os internos.
  std::uint32_t voice_count(SoundId id) const;

  // EN: Sets the per-sound volume. Rejects a non-finite (NaN/Inf) or negative `v`, keeping the
  //     sound's previous volume unchanged; otherwise clamps to [0, 1]. Applies to the PRIMARY
  //     instance AND every currently-live oneshot copy of `id` (least surprise -- a copy created
  //     AFTER this call still inherits the primary's volume at ITS OWN creation time, per
  //     `play_oneshot()`'s contract, not retroactively from this call). Returns false when not
  //     initialized, `id` is unknown/0, or `v` was rejected; true on an accepted (possibly
  //     clamped) value.
  // PT: Define o volume por-som. Rejeita `v` não-finito (NaN/Inf) ou negativo, mantendo o
  //     volume anterior do som inalterado; caso contrário faz clamp em [0, 1]. Aplica na
  //     instância PRIMÁRIA E em toda cópia oneshot viva de `id` no momento (menor surpresa -- uma
  //     cópia criada DEPOIS desta chamada ainda herda o volume da primária no MOMENTO DELA
  //     própria, conforme o contrato do `play_oneshot()`, não retroativamente desta chamada).
  //     Retorna false quando não inicializado, `id` é desconhecido/0, ou `v` foi rejeitado; true
  //     num valor aceito (possivelmente clampeado).
  bool set_volume(SoundId id, float v);

  // EN: Sets the engine-wide master volume. Same validation contract as set_volume() (rejects
  //     non-finite/negative, keeps previous, clamps to [0, 1]). No-op (does nothing, no crash)
  //     when not initialized.
  // PT: Define o volume master do engine inteiro. Mesmo contrato de validação do set_volume()
  //     (rejeita não-finito/negativo, mantém o anterior, clampeia em [0, 1]). No-op (não faz
  //     nada, sem crash) quando não inicializado.
  void set_master_volume(float v);

  // EN: Stops every currently loaded sound -- every primary AND every live oneshot copy (does not
  //     unload any of them). No-op when not initialized.
  // PT: Para todo som carregado no momento -- toda primária E toda cópia oneshot viva (não
  //     descarrega nenhum). No-op quando não inicializado.
  void stop_all();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
