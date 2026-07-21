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
//     POLYPHONY (deferred, declared): one live instance per `SoundId`. Calling `play()` on a
//     `SoundId` that is already playing restarts it from frame 0 -- overlapping instances of the
//     SAME sound effect (e.g. rapid-fire gunshots) is a future slice (`ma_sound_init_copy`),
//     documented in docs/audio.md, not silently half-implemented here.
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
//     POLIFONIA (adiada, declarada): uma instância viva por `SoundId`. Chamar `play()` num
//     `SoundId` que já está tocando reinicia do frame 0 -- instâncias sobrepostas do MESMO
//     efeito sonoro (ex.: tiros em rajada) é fatia futura (`ma_sound_init_copy`), documentado em
//     docs/audio.md, não meio-implementado silenciosamente aqui.
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
  //     playback of `id`. Returns false when not initialized or `id` is unknown/0.
  // PT: Inicia (ou reinicia do frame 0, se já estiver tocando -- ver a nota de polifonia acima)
  //     a reprodução de `id`. Retorna false quando não inicializado ou `id` é desconhecido/0.
  bool play(SoundId id);

  // EN: Stops playback of `id` (does not unload it -- play() again resumes from frame 0).
  //     Returns false when not initialized or `id` is unknown/0.
  // PT: Para a reprodução de `id` (não descarrega -- play() de novo retoma do frame 0). Retorna
  //     false quando não inicializado ou `id` é desconhecido/0.
  bool stop(SoundId id);

  // EN: Sets the per-sound volume. Rejects a non-finite (NaN/Inf) or negative `v`, keeping the
  //     sound's previous volume unchanged; otherwise clamps to [0, 1]. Returns false when not
  //     initialized, `id` is unknown/0, or `v` was rejected; true on an accepted (possibly
  //     clamped) value.
  // PT: Define o volume por-som. Rejeita `v` não-finito (NaN/Inf) ou negativo, mantendo o
  //     volume anterior do som inalterado; caso contrário faz clamp em [0, 1]. Retorna false
  //     quando não inicializado, `id` é desconhecido/0, ou `v` foi rejeitado; true num valor
  //     aceito (possivelmente clampeado).
  bool set_volume(SoundId id, float v);

  // EN: Sets the engine-wide master volume. Same validation contract as set_volume() (rejects
  //     non-finite/negative, keeps previous, clamps to [0, 1]). No-op (does nothing, no crash)
  //     when not initialized.
  // PT: Define o volume master do engine inteiro. Mesmo contrato de validação do set_volume()
  //     (rejeita não-finito/negativo, mantém o anterior, clampeia em [0, 1]). No-op (não faz
  //     nada, sem crash) quando não inicializado.
  void set_master_volume(float v);

  // EN: Stops every currently loaded sound (does not unload any of them). No-op when not
  //     initialized.
  // PT: Para todo som carregado no momento (não descarrega nenhum). No-op quando não
  //     inicializado.
  void stop_all();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
