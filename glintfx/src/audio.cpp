// SPDX-License-Identifier: MPL-2.0
// EN: Audio implementation (A3-AUDIO) -- the pimpl `Impl` is where every miniaudio (`ma_*`) type
//     actually lives; nothing miniaudio-shaped crosses glintfx/audio.hpp. See that header's
//     comment for the full lifecycle/RAII/real-time-safety/hardening contract this file
//     implements; comments here focus on the "how", not the "why" (already spelled out there).
//
//     MA_NO_ENCODING/MA_NO_GENERATION are re-defined identically here (not just in
//     miniaudio_impl.c) because miniaudio.h's DECLARATIONS section (everything above the
//     `#if defined(MINIAUDIO_IMPLEMENTATION)` guard) is itself gated by these same macros --
//     this translation unit only ever includes miniaudio.h for declarations (no
//     MINIAUDIO_IMPLEMENTATION here), but the declared API surface must match what
//     miniaudio_impl.c actually defines.
// PT: Implementação do Audio (A3-AUDIO) -- o pimpl `Impl` é onde todo tipo miniaudio (`ma_*`) de
//     fato vive; nada com formato de miniaudio cruza o glintfx/audio.hpp. Ver o comentário
//     daquele header pro contrato completo de ciclo-de-vida/RAII/segurança-real-time/hardening
//     que este arquivo implementa; os comentários aqui focam no "como", não no "porquê" (já
//     detalhado lá).
//
//     MA_NO_ENCODING/MA_NO_GENERATION são redefinidos identicamente aqui (não só no
//     miniaudio_impl.c) porque a seção de DECLARAÇÕES do miniaudio.h (tudo acima do guard
//     `#if defined(MINIAUDIO_IMPLEMENTATION)`) é ela mesma gateada por essas mesmas macros --
//     esta unidade de tradução só inclui o miniaudio.h para declarações (sem
//     MINIAUDIO_IMPLEMENTATION aqui), mas a superfície de API declarada precisa bater com o que
//     o miniaudio_impl.c de fato define.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/audio.hpp>

#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include "miniaudio.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace glintfx {

namespace {

// EN: Hard cap on live oneshot copies per SoundId (AUDIO-F2 §2) -- once reached (after the lazy
//     reap runs), the OLDEST copy is stolen (uninit + dropped) so the newest hit is always
//     audible. Standard, deterministic audio voice-stealing; not user-configurable in this slice
//     (no consumer need surfaced yet -- revisit only if one does).
// PT: Teto rígido de cópias oneshot vivas por SoundId (AUDIO-F2 §2) -- ao atingir (depois do
//     reap preguiçoso rodar), a cópia MAIS ANTIGA é roubada (uninit + descartada) pra que o hit
//     mais novo sempre soe. Voice-stealing de áudio padrão e determinístico; não configurável
//     pelo usuário nesta fatia (nenhuma necessidade de consumidor surgiu ainda -- revisitar só se
//     surgir).
constexpr std::size_t kMaxVoicesPerSound = 32;

// EN: Upper bound accepted for any fade duration (play()'s fade_in_s, stop()'s fade_out_s) --
//     house hardening rule: reject non-finite/negative outright, clamp everything else into a
//     sane range instead of trusting an attacker- or bug-reachable float all the way to the
//     ma_uint64 millisecond cast.
// PT: Teto aceito pra qualquer duração de fade (fade_in_s do play(), fade_out_s do stop()) --
//     regra de hardening da casa: rejeita não-finito/negativo de cara, clampeia o resto num
//     intervalo são em vez de confiar num float alcançável por atacante/bug até o cast final pra
//     milissegundos ma_uint64.
constexpr float kMaxFadeSeconds = 3600.0f;

// EN: Validates a fade duration (seconds) per the module's hardening contract (see audio.hpp):
//     rejects (`false`, `out_seconds` left untouched) when `!std::isfinite(seconds) || seconds <
//     0`; otherwise clamps into `[0, kMaxFadeSeconds]` and writes the clamped value. Deliberately
//     the ONLY place that touches `seconds` before it is cast to `ma_uint64` milliseconds
//     (`fade_seconds_to_ms` below) -- validating the float strictly BEFORE that cast is the exact
//     ordering the audit-domino memory names (an unchecked float -> integer cast is the bug
//     class, not a hypothetical).
// PT: Valida uma duração de fade (segundos) conforme o contrato de hardening do módulo (ver
//     audio.hpp): rejeita (`false`, `out_seconds` intocado) quando `!std::isfinite(seconds) ||
//     seconds < 0`; caso contrário clampeia em `[0, kMaxFadeSeconds]` e escreve o valor
//     clampeado. Deliberadamente o ÚNICO lugar que toca em `seconds` antes do cast pra
//     milissegundos `ma_uint64` (`fade_seconds_to_ms` abaixo) -- validar o float estritamente
//     ANTES desse cast é a ordem exata que a memória de auditoria-dominó nomeia (um cast
//     float -> inteiro sem checagem é a classe de bug, não uma hipótese).
bool validate_fade_seconds(float seconds, float& out_seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0f) {
    return false;
  }
  out_seconds = std::min(seconds, kMaxFadeSeconds);
  return true;
}

// EN: Converts an ALREADY-VALIDATED (see validate_fade_seconds above) fade duration to the
//     ma_uint64 milliseconds every miniaudio fade/stop-with-fade call in this file takes. Safe by
//     construction: `seconds` is clamped to `[0, 3600]` before this is ever called, so the
//     largest possible result (3,600,000) is nowhere near ma_uint64's range.
// PT: Converte uma duração de fade JÁ VALIDADA (ver validate_fade_seconds acima) pros
//     milissegundos ma_uint64 que toda chamada de fade/stop-com-fade do miniaudio neste arquivo
//     recebe. Seguro por construção: `seconds` já está clampeado em `[0, 3600]` antes disto ser
//     chamado, então o maior resultado possível (3.600.000) está longe do limite do ma_uint64.
ma_uint64 fade_seconds_to_ms(float seconds) {
  return static_cast<ma_uint64>(seconds * 1000.0f);
}

// EN: Stops one ma_sound (primary OR a copy) immediately (fade_seconds == 0) or schedules a
//     fade-out (fade_seconds > 0) -- shared by stop()'s primary+every-live-copy loop.
// PT: Para um ma_sound (primária OU uma cópia) imediatamente (fade_seconds == 0) ou agenda um
//     fade-out (fade_seconds > 0) -- compartilhado pelo laço primária+toda-cópia-viva do stop().
bool stop_one(ma_sound* snd, float fade_seconds) {
  if (fade_seconds <= 0.0f) {
    return ma_sound_stop(snd) == MA_SUCCESS;
  }
  return ma_sound_stop_with_fade_in_milliseconds(snd, fade_seconds_to_ms(fade_seconds)) == MA_SUCCESS;
}

// EN: Reaps every copy of one SoundId's voice pool whose playback has naturally finished
//     (`ma_sound_at_end()` true): `ma_sound_uninit()`s it and drops it from `copies`. Called ONLY
//     from `play_oneshot()` -- see audio.hpp's threading contract and this file's `shutdown()`
//     ordering comment: single-threaded by construction (both the reap and shutdown() run on the
//     caller's own thread), so this cannot race the teardown path.
// PT: Reapa toda cópia do pool de vozes de um SoundId cuja reprodução terminou naturalmente
//     (`ma_sound_at_end()` true): dá `ma_sound_uninit()` nela e descarta de `copies`. Chamado SÓ
//     do `play_oneshot()` -- ver o contrato de threading do audio.hpp e o comentário de ordem do
//     `shutdown()` deste arquivo: single-thread por construção (tanto o reap quanto o shutdown()
//     rodam na própria thread chamadora), então isto não pode correr com o caminho de teardown.
void reap_finished_voices(std::vector<std::unique_ptr<ma_sound>>& copies) {
  copies.erase(
      std::remove_if(copies.begin(), copies.end(),
                      [](const std::unique_ptr<ma_sound>& copy) {
                        if (ma_sound_at_end(copy.get())) {
                          ma_sound_uninit(copy.get());
                          return true;
                        }
                        return false;
                      }),
      copies.end());
}

} // namespace

// EN: Everything miniaudio-shaped: the (optional, null-backend-only) context, the engine, and
//     one ma_sound per loaded SoundId. `sounds` uses std::unique_ptr<ma_sound> (not ma_sound by
//     value) so a rehash of the unordered_map never moves/invalidates an ma_sound's address --
//     miniaudio stores internal self-referential state inside ma_sound, same "stable address"
//     concern DataBinder documents for its own cell map (src/data_binder.hpp).
// PT: Tudo com formato de miniaudio: o contexto (opcional, só pra null-backend), o engine, e um
//     ma_sound por SoundId carregado. `sounds` usa std::unique_ptr<ma_sound> (não ma_sound por
//     valor) para que um rehash do unordered_map nunca mova/invalide o endereço de um ma_sound
//     -- o miniaudio guarda estado interno auto-referencial dentro do ma_sound, mesma
//     preocupação de "endereço estável" que o DataBinder documenta pro próprio mapa de células
//     (src/data_binder.hpp).
struct Audio::Impl {
  ma_context context{};
  bool       context_initialized = false;

  // EN: A resource manager we own and configure with jobThreadCount = 0 (see init()'s comment
  //     for why: it makes ma_sound_init_from_file()+MA_SOUND_FLAG_DECODE do its file-read+decode
  //     work entirely on the CALLING thread, synchronously, with zero background job threads --
  //     the literal, structural form of this module's "no streaming/async decode, nothing of
  //     ours runs on the audio thread" real-time-safety claim in audio.hpp).
  // PT: Um resource manager que possuímos e configuramos com jobThreadCount = 0 (ver o
  //     comentário do init() pro porquê: faz o ma_sound_init_from_file()+MA_SOUND_FLAG_DECODE
  //     fazer o trabalho de leitura+decode inteiramente na thread CHAMADORA, de forma síncrona,
  //     com zero threads de job em segundo plano -- a forma literal e estrutural da alegação de
  //     segurança-real-time "sem decode em streaming/assíncrono, nada nosso roda na audio
  //     thread" do audio.hpp).
  ma_resource_manager resource_manager{};
  bool                resource_manager_initialized = false;

  ma_engine engine{};
  bool      engine_initialized = false;

  std::unordered_map<SoundId, std::unique_ptr<ma_sound>> sounds;

  // EN: AUDIO-F2 -- live oneshot copies per SoundId (see audio.hpp's POLYPHONY paragraph). Same
  //     "stable address" reasoning as `sounds` above applies per-element
  //     (`std::unique_ptr<ma_sound>`, not `ma_sound` by value): ma_sound is self-referential, so
  //     a vector reallocation must not move an already-registered copy. An id with no entry here
  //     (or an empty vector) simply has zero live copies -- `voices` only ever gains an entry for
  //     an id the first time `play_oneshot(id)` succeeds for it.
  // PT: AUDIO-F2 -- cópias oneshot vivas por SoundId (ver o parágrafo POLIFONIA do audio.hpp).
  //     Mesmo raciocínio de "endereço estável" do `sounds` acima se aplica por-elemento
  //     (`std::unique_ptr<ma_sound>`, não `ma_sound` por valor): ma_sound é auto-referencial,
  //     então uma realocação do vector não pode mover uma cópia já registrada. Um id sem entrada
  //     aqui (ou com vector vazio) simplesmente tem zero cópias vivas -- `voices` só ganha uma
  //     entrada pra um id na primeira vez que `play_oneshot(id)` tem sucesso pra ele.
  std::unordered_map<SoundId, std::vector<std::unique_ptr<ma_sound>>> voices;

  // EN: Monotonic, NEVER reset (not even across a shutdown()+init() cycle within the same Audio
  //     instance -- see audio.hpp's SoundId doc-comment for why this matters).
  // PT: Monotônico, NUNCA reiniciado (nem entre um ciclo shutdown()+init() dentro da mesma
  //     instância Audio -- ver o doc-comment de SoundId no audio.hpp pra entender por que
  //     importa).
  SoundId next_id = 1;
};

Audio::Audio() : impl_(std::make_unique<Impl>()) {}

Audio::~Audio() {
  shutdown();
}

bool Audio::init(const AudioConfig& cfg) {
  if (impl_->engine_initialized) {
    return false; // EN: already initialized -- call shutdown() first. PT: já inicializado -- chame shutdown() primeiro.
  }

  // EN: jobThreadCount = 0 -- self-managed, i.e. NO internal background job thread. Without
  //     this, miniaudio's default resource manager (jobThreadCount defaults to 1) decodes
  //     MA_SOUND_FLAG_DECODE sounds via an internal worker thread and a completion fence; under
  //     ASan/ThreadSanitizer this was observed to trip a genuine heap-use-after-free race
  //     inside miniaudio's own ma_resource_manager_data_buffer_node_acquire() (confirmed via
  //     this module's own review-adversarial pass, both on a well-formed AND a hostile input
  //     load). Forcing 0 job threads makes ma_sound_init_from_file() perform the read+decode
  //     synchronously on the CALLING thread -- exactly this module's documented real-time-safety
  //     contract (no async decode, no thread of ours doing I/O behind our back), and the race is
  //     gone because there is no second thread to race with.
  // PT: jobThreadCount = 0 -- autogerenciado, ou seja, SEM thread de job em segundo plano
  //     interna. Sem isso, o resource manager padrão do miniaudio (jobThreadCount default 1)
  //     decodifica sons MA_SOUND_FLAG_DECODE via uma worker thread interna e uma fence de
  //     conclusão; sob ASan/ThreadSanitizer isso foi observado disparando uma race genuína de
  //     heap-use-after-free dentro do próprio ma_resource_manager_data_buffer_node_acquire() do
  //     miniaudio (confirmado no próprio review-adversarial deste módulo, tanto num load
  //     bem-formado QUANTO num hostil). Forçar 0 threads de job faz o ma_sound_init_from_file()
  //     fazer a leitura+decode de forma síncrona na thread CHAMADORA -- exatamente o contrato de
  //     segurança-real-time documentado deste módulo (sem decode assíncrono, nenhuma thread
  //     nossa fazendo I/O nas costas), e a race desaparece porque não há uma segunda thread pra
  //     competir.
  ma_resource_manager_config resource_manager_cfg = ma_resource_manager_config_init();
  resource_manager_cfg.jobThreadCount              = 0;
  if (ma_resource_manager_init(&resource_manager_cfg, &impl_->resource_manager) != MA_SUCCESS) {
    return false;
  }
  impl_->resource_manager_initialized = true;

  ma_engine_config engine_cfg   = ma_engine_config_init();
  engine_cfg.pResourceManager   = &impl_->resource_manager;

  if (cfg.null_backend) {
    // EN: A private ma_context pinned to the null backend -- the headless-CI path (no
    //     ALSA/PulseAudio/Xvfb needed). Must outlive the engine (torn down AFTER
    //     ma_engine_uninit() in shutdown(), mirroring the sounds-before-engine ordering).
    // PT: Um ma_context privado fixado no backend null -- o caminho de CI headless (sem
    //     ALSA/PulseAudio/Xvfb necessário). Precisa sobreviver ao engine (desmontado DEPOIS de
    //     ma_engine_uninit() em shutdown(), espelhando a ordem "sons antes do engine").
    ma_backend             backends[] = {ma_backend_null};
    ma_context_config      ctx_cfg    = ma_context_config_init();
    if (ma_context_init(backends, 1, &ctx_cfg, &impl_->context) != MA_SUCCESS) {
      ma_resource_manager_uninit(&impl_->resource_manager);
      impl_->resource_manager_initialized = false;
      return false;
    }
    impl_->context_initialized = true;
    engine_cfg.pContext        = &impl_->context;
  }

  if (ma_engine_init(&engine_cfg, &impl_->engine) != MA_SUCCESS) {
    if (impl_->context_initialized) {
      ma_context_uninit(&impl_->context);
      impl_->context_initialized = false;
    }
    ma_resource_manager_uninit(&impl_->resource_manager);
    impl_->resource_manager_initialized = false;
    return false;
  }

  impl_->engine_initialized = true;
  return true;
}

void Audio::shutdown() {
  if (!impl_->engine_initialized) {
    return; // EN: idempotent no-op. PT: no-op idempotente.
  }

  // EN: THE non-negotiable ordering (see audio.hpp's header comment; extended for AUDIO-F2's
  //     polyphony -- see the `voices` block right below): every oneshot COPY is uninitialized
  //     first, then every PRIMARY ma_sound, both BEFORE ma_engine_uninit() tears down the
  //     device/context any of them would otherwise still be referencing. Copies-before-primaries
  //     matters too, not just sounds-before-engine: a copy shares a refcounted data-buffer node in
  //     the resource manager with the primary it was cloned from (`ma_resource_manager_data_
  //     source_init_copy()`), so the primary's node must still be alive while its copies release
  //     their reference into it -- releasing primaries first would leave copies pointing at an
  //     already-torn-down node for however long they take to reach their own ma_sound_uninit().
  //
  //     ASAN BLIND SPOT (review adversarial finding #1, S4, non-blocking but load-bearing to
  //     know; the same reasoning now covers `voices` too): this ordering canNOT be verified by
  //     flipping it and expecting ASan to go red.
  //     `impl_->engine` is a plain member of `Impl` (stack/heap-embedded by whoever owns the
  //     `Audio`, e.g. via `std::make_unique<Impl>()`), NOT a separately heap-allocated object
  //     `ma_engine_uninit()` itself `free()`s -- miniaudio's teardown releases the engine's
  //     INTERNAL resources (device, resource-manager link, node graph) but never returns the
  //     `ma_engine*` memory to the allocator, because it never owned that memory in the first
  //     place. So a future refactor that swaps this to "engine first, then sounds" would NOT
  //     produce a use-after-free ASan can catch: `ma_sound_uninit()` called after
  //     `ma_engine_uninit()` still touches perfectly live, un-poisoned memory (the `ma_sound`
  //     struct and the `ma_engine` struct it points back into are both still allocated, just
  //     torn down internally) -- the bug would be silent memory-safety-tool-invisible breakage
  //     (undefined/backend-dependent behaviour reading a half-torn-down engine, not a crash),
  //     caught only by manual code review of this exact ordering. If you touch this function,
  //     re-read this comment before moving a single line.
  // PT: A ordem inegociável (ver o comentário de cabeçalho do audio.hpp; estendida pra polifonia
  //     do AUDIO-F2 -- ver o bloco `voices` logo abaixo): toda cópia oneshot é desinicializada
  //     primeiro, depois toda primária ma_sound, ambos ANTES de ma_engine_uninit() derrubar o
  //     device/contexto que qualquer uma delas estaria referenciando. Cópias-antes-de-primárias
  //     importa também, não só sons-antes-do-engine: uma cópia compartilha um data-buffer node
  //     refcontado no resource manager com a primária de onde foi clonada
  //     (`ma_resource_manager_data_source_init_copy()`), então o node da primária precisa
  //     continuar vivo enquanto as cópias liberam a referência nele -- liberar primárias primeiro
  //     deixaria cópias apontando pra um node já desmontado pelo tempo que levassem até seu
  //     próprio ma_sound_uninit().
  //
  //     PONTO CEGO DO ASAN (achado #1 do review adversarial, S4, não-bloqueante mas importante
  //     de saber; o mesmo raciocínio agora cobre `voices` também): esta ordem NÃO pode ser
  //     verificada invertendo-a e esperando o ASan acusar.
  //     `impl_->engine` é um membro simples de `Impl` (embutido em stack/heap por quem for dono
  //     do `Audio`, ex.: via `std::make_unique<Impl>()`), NÃO um objeto alocado separadamente no
  //     heap que o próprio `ma_engine_uninit()` dá `free()` -- o teardown do miniaudio libera os
  //     recursos INTERNOS do engine (device, ligação com o resource manager, node graph) mas
  //     nunca devolve a memória do `ma_engine*` pro alocador, porque ele nunca foi dono dessa
  //     memória. Então um refactor futuro que trocar pra "engine primeiro, depois os sons" NÃO
  //     produziria um use-after-free que o ASan pega: `ma_sound_uninit()` chamado depois de
  //     `ma_engine_uninit()` ainda toca memória perfeitamente viva e não-envenenada (a struct
  //     `ma_sound` e a struct `ma_engine` pra qual ela aponta de volta continuam alocadas, só
  //     desmontadas internamente) -- o bug seria uma quebra silenciosa, invisível pra ferramenta
  //     de memory-safety (comportamento indefinido/dependente-de-backend lendo um engine
  //     meio-desmontado, não um crash), pega só por review manual desta ordem exata. Se você
  //     mexer nesta função, releia este comentário antes de mover uma linha sequer.
  for (auto& entry : impl_->voices) {
    for (auto& copy : entry.second) {
      ma_sound_uninit(copy.get());
    }
  }
  impl_->voices.clear();

  for (auto& entry : impl_->sounds) {
    ma_sound_uninit(entry.second.get());
  }
  impl_->sounds.clear();

  ma_engine_uninit(&impl_->engine);
  impl_->engine_initialized = false;

  // EN: We supplied our OWN resource manager (init()'s pResourceManager), so
  //     ma_engine_uninit() above did NOT tear it down (ma_engine only uninits a resource
  //     manager it created itself) -- ours is torn down explicitly here, after the engine that
  //     used it, before the context.
  // PT: Fornecemos NOSSO PRÓPRIO resource manager (pResourceManager do init()), então o
  //     ma_engine_uninit() acima NÃO o desmontou (o ma_engine só desmonta um resource manager
  //     que ele mesmo criou) -- o nosso é desmontado explicitamente aqui, depois do engine que o
  //     usava, antes do contexto.
  if (impl_->resource_manager_initialized) {
    ma_resource_manager_uninit(&impl_->resource_manager);
    impl_->resource_manager_initialized = false;
  }

  if (impl_->context_initialized) {
    ma_context_uninit(&impl_->context);
    impl_->context_initialized = false;
  }
}

bool Audio::is_initialized() const {
  return impl_->engine_initialized;
}

Audio::SoundId Audio::load_sound(const char* path) {
  if (!impl_->engine_initialized || path == nullptr) {
    return 0;
  }

  auto sound = std::make_unique<ma_sound>();
  // EN: MA_SOUND_FLAG_DECODE -- full synchronous decode NOW, not streamed later. This is the
  //     structural guarantee behind audio.hpp's real-time-safety claim: the audio callback
  //     thread only ever touches already-decoded PCM. On any failure (missing/unreadable file,
  //     truncated/corrupt/unrecognised content) miniaudio returns non-MA_SUCCESS and this
  //     function returns 0 WITHOUT registering a partially-constructed sound -- `sound` is
  //     destroyed here (unique_ptr going out of scope), never leaked, never left half-alive in
  //     `impl_->sounds`.
  // PT: MA_SOUND_FLAG_DECODE -- decode completo e síncrono AGORA, não em streaming depois. Essa
  //     é a garantia estrutural por trás da alegação de segurança-real-time do audio.hpp: a
  //     thread de callback de áudio só toca PCM já decodificado. Em qualquer falha (arquivo
  //     ausente/ilegível, conteúdo truncado/corrompido/não-reconhecido) o miniaudio retorna
  //     não-MA_SUCCESS e esta função retorna 0 SEM registrar um som parcialmente construído --
  //     `sound` é destruído aqui (unique_ptr saindo de escopo), nunca vazado, nunca deixado
  //     meio-vivo em `impl_->sounds`.
  ma_result result = ma_sound_init_from_file(
      &impl_->engine, path, MA_SOUND_FLAG_DECODE, /*pGroup=*/nullptr, /*pDoneFence=*/nullptr,
      sound.get());
  if (result != MA_SUCCESS) {
    return 0;
  }

  const SoundId id = impl_->next_id++;
  impl_->sounds.emplace(id, std::move(sound));
  return id;
}

bool Audio::play(SoundId id, bool loop, float fade_in_s) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  float fade_seconds = 0.0f;
  if (!validate_fade_seconds(fade_in_s, fade_seconds)) {
    return false; // EN: hostile fade, primary left untouched. PT: fade hostil, primária intocada.
  }

  ma_sound* snd = it->second.get();

  // EN: Reset any stop-time+fade scheduled by a PREVIOUS stop(id, fade_out_s) BEFORE seek+start
  //     -- the exact trap the vendored miniaudio.h's own declaration comment warns about
  //     (`ma_sound_stop_with_fade_in_milliseconds`'s doc: "if you want to restart the sound,
  //     first reset it with ma_sound_reset_stop_time_and_fade()"): without this, a scheduled
  //     future stop time silently kills THIS fresh playback once it arrives. Mandatory regression
  //     test: audio_loop_fade_sanity.cpp.
  // PT: Reseta qualquer stop-time+fade agendado por um stop(id, fade_out_s) ANTERIOR ANTES do
  //     seek+start -- exatamente a armadilha que o próprio comentário de declaração do
  //     miniaudio.h vendorizado avisa (doc de `ma_sound_stop_with_fade_in_milliseconds`: "se você
  //     quer reiniciar o som, primeiro resete com ma_sound_reset_stop_time_and_fade()"): sem
  //     isso, um stop-time futuro agendado mata silenciosamente ESTA reprodução nova assim que
  //     chega. Teste de regressão obrigatório: audio_loop_fade_sanity.cpp.
  ma_sound_reset_stop_time_and_fade(snd);
  ma_sound_set_looping(snd, loop);

  if (fade_seconds > 0.0f) {
    // EN: Fade from silence up to the sound's CURRENT volume (not a hardcoded 1.0) -- so a prior
    //     set_volume(id, 0.5f) survives the fade-in (see audio.hpp). ma_sound_get_volume() reads
    //     the node's plain volume field, independent of the fader state we are about to
    //     overwrite, so read-then-set here is safe regardless of ordering.
    // PT: Fade do silêncio até o volume ATUAL do som (não um 1.0 fixo) -- pra um set_volume(id,
    //     0.5f) anterior sobreviver ao fade-in (ver audio.hpp). ma_sound_get_volume() lê o campo
    //     de volume simples do node, independente do estado do fader que estamos prestes a
    //     sobrescrever, então ler-depois-setar aqui é seguro independente da ordem.
    const float current_volume = ma_sound_get_volume(snd);
    ma_sound_set_fade_in_milliseconds(snd, 0.0f, current_volume, fade_seconds_to_ms(fade_seconds));
  }

  // EN: Always seek to frame 0 before starting -- the single well-defined "restart" contract
  //     documented in audio.hpp, independent of whether the sound was already playing, already
  //     stopped mid-way, or had already reached its end.
  // PT: Sempre volta pro frame 0 antes de iniciar -- o único contrato de "reinício" bem
  //     definido documentado no audio.hpp, independente de o som já estar tocando, já ter
  //     parado no meio, ou já ter chegado ao fim.
  ma_sound_seek_to_pcm_frame(snd, 0);
  return ma_sound_start(snd) == MA_SUCCESS;
}

bool Audio::stop(SoundId id, float fade_out_s) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  float fade_seconds = 0.0f;
  if (!validate_fade_seconds(fade_out_s, fade_seconds)) {
    return false; // EN: hostile fade, nothing stopped. PT: fade hostil, nada parado.
  }

  const bool primary_ok = stop_one(it->second.get(), fade_seconds);

  // EN: A fade-out (or an immediate stop) applies to every live copy of this id too -- least
  //     surprise, and the copies would otherwise keep sounding after the "same effect" appears to
  //     have stopped.
  // PT: Um fade-out (ou stop imediato) se aplica em toda cópia viva deste id também -- menor
  //     surpresa, senão as cópias continuariam soando depois do "mesmo efeito" parecer ter
  //     parado.
  auto voices_it = impl_->voices.find(id);
  if (voices_it != impl_->voices.end()) {
    for (auto& copy : voices_it->second) {
      stop_one(copy.get(), fade_seconds);
    }
  }
  return primary_ok;
}

bool Audio::set_looping(SoundId id, bool loop) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  ma_sound_set_looping(it->second.get(), loop);
  return true;
}

bool Audio::play_oneshot(SoundId id) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }

  // EN: operator[] creates an empty vector the first time this id gets a oneshot -- fine, an
  //     empty pool is the correct "zero live copies yet" state.
  // PT: operator[] cria um vector vazio na primeira vez que este id ganha um oneshot -- correto,
  //     um pool vazio é o estado certo de "zero cópias vivas ainda".
  auto& copies = impl_->voices[id];

  // EN: Lazy reap FIRST (see audio.hpp's threading contract + this file's shutdown() comment:
  //     single-threaded by construction, cannot race shutdown()).
  // PT: Reap preguiçoso PRIMEIRO (ver o contrato de threading do audio.hpp + o comentário do
  //     shutdown() deste arquivo: single-thread por construção, não pode correr com o
  //     shutdown()).
  reap_finished_voices(copies);

  // EN: Still at the cap after reaping -- steal the OLDEST copy (front of the vector: copies are
  //     always push_back()'d, so index 0 is the oldest survivor) so the newest hit is always
  //     audible. Deterministic voice-stealing; play_oneshot() itself never fails because of this
  //     cap.
  // PT: Ainda no teto depois do reap -- rouba a cópia MAIS ANTIGA (início do vector: cópias são
  //     sempre push_back()'d, então o índice 0 é a sobrevivente mais antiga) pra que o hit mais
  //     novo sempre soe. Voice-stealing determinístico; o play_oneshot() em si nunca falha por
  //     causa deste teto.
  if (copies.size() >= kMaxVoicesPerSound) {
    ma_sound_uninit(copies.front().get());
    copies.erase(copies.begin());
  }

  auto copy = std::make_unique<ma_sound>();
  if (ma_sound_init_copy(&impl_->engine, it->second.get(), /*flags=*/0, /*pGroup=*/nullptr,
                          copy.get()) != MA_SUCCESS) {
    return false;
  }

  // EN: ma_sound_init_copy() does NOT copy volume (verified against the vendored
  //     implementation: it copies monoExpansionMode/volumeSmoothTimeInPCMFrames, nothing about
  //     volume) -- apply the primary's CURRENT volume explicitly so a copy played mid-fade or
  //     after a set_volume() sounds like the primary, not silently at gain 1.0. Copies never
  //     loop -- a looping orphan copy could never be reaped (deliberate footgun removal, see
  //     audio.hpp).
  // PT: ma_sound_init_copy() NÃO copia volume (verificado contra a implementação vendorizada:
  //     copia monoExpansionMode/volumeSmoothTimeInPCMFrames, nada de volume) -- aplica o volume
  //     ATUAL da primária explicitamente, pra uma cópia tocada em meio a um fade ou depois de um
  //     set_volume() soar como a primária, não silenciosamente no ganho 1.0. Cópias nunca loopam
  //     -- uma cópia órfã em loop jamais seria reapada (remoção deliberada de footgun, ver
  //     audio.hpp).
  ma_sound_set_volume(copy.get(), ma_sound_get_volume(it->second.get()));
  ma_sound_set_looping(copy.get(), MA_FALSE);
  ma_sound_seek_to_pcm_frame(copy.get(), 0);

  if (ma_sound_start(copy.get()) != MA_SUCCESS) {
    ma_sound_uninit(copy.get());
    return false;
  }

  copies.push_back(std::move(copy));
  return true;
}

bool Audio::is_playing(SoundId id) const {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  if (ma_sound_is_playing(it->second.get())) {
    return true;
  }
  auto voices_it = impl_->voices.find(id);
  if (voices_it != impl_->voices.end()) {
    for (auto& copy : voices_it->second) {
      if (ma_sound_is_playing(copy.get())) {
        return true;
      }
    }
  }
  return false;
}

std::uint32_t Audio::voice_count(SoundId id) const {
  if (!impl_->engine_initialized || id == 0) {
    return 0;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return 0;
  }
  std::uint32_t count = ma_sound_is_playing(it->second.get()) ? 1u : 0u;
  auto voices_it = impl_->voices.find(id);
  if (voices_it != impl_->voices.end()) {
    for (auto& copy : voices_it->second) {
      if (ma_sound_is_playing(copy.get())) {
        ++count;
      }
    }
  }
  return count;
}

bool Audio::set_volume(SoundId id, float v) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  if (!std::isfinite(v) || v < 0.0f) {
    return false; // EN: reject, keep previous (we simply never touch it). PT: rejeita, mantém o anterior (simplesmente nunca tocamos nele).
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  const float clamped = std::min(v, 1.0f);
  ma_sound_set_volume(it->second.get(), clamped);

  // EN: Applies to every currently-live copy too (least surprise) -- a copy created AFTER this
  //     call is unaffected by it (play_oneshot() reads the primary's volume at ITS OWN creation
  //     time, not this call's value, since this call has already landed on the primary by then).
  // PT: Aplica em toda cópia viva no momento também (menor surpresa) -- uma cópia criada DEPOIS
  //     desta chamada não é afetada por ela (play_oneshot() lê o volume da primária no MOMENTO
  //     DELA própria, não o valor desta chamada, já que esta chamada já terá aterrissado na
  //     primária até lá).
  auto voices_it = impl_->voices.find(id);
  if (voices_it != impl_->voices.end()) {
    for (auto& copy : voices_it->second) {
      ma_sound_set_volume(copy.get(), clamped);
    }
  }
  return true;
}

void Audio::set_master_volume(float v) {
  if (!impl_->engine_initialized) {
    return;
  }
  if (!std::isfinite(v) || v < 0.0f) {
    return; // EN: reject, keep previous. PT: rejeita, mantém o anterior.
  }
  ma_engine_set_volume(&impl_->engine, std::min(v, 1.0f));
}

void Audio::stop_all() {
  if (!impl_->engine_initialized) {
    return;
  }
  for (auto& entry : impl_->sounds) {
    ma_sound_stop(entry.second.get());
  }
  for (auto& entry : impl_->voices) {
    for (auto& copy : entry.second) {
      ma_sound_stop(copy.get());
    }
  }
}

} // namespace glintfx
