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

namespace glintfx {

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

  // EN: THE non-negotiable ordering (see audio.hpp's header comment): every ma_sound is
  //     uninitialized BEFORE ma_engine_uninit() tears down the device/context a live ma_sound
  //     would otherwise still be referencing.
  // PT: A ordem inegociável (ver o comentário de cabeçalho do audio.hpp): todo ma_sound é
  //     desinicializado ANTES de ma_engine_uninit() derrubar o device/contexto que um ma_sound
  //     vivo estaria referenciando.
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

bool Audio::play(SoundId id) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  // EN: Always seek to frame 0 before starting -- the single well-defined "restart" contract
  //     documented in audio.hpp, independent of whether the sound was already playing, already
  //     stopped mid-way, or had already reached its end.
  // PT: Sempre volta pro frame 0 antes de iniciar -- o único contrato de "reinício" bem
  //     definido documentado no audio.hpp, independente de o som já estar tocando, já ter
  //     parado no meio, ou já ter chegado ao fim.
  ma_sound_seek_to_pcm_frame(it->second.get(), 0);
  return ma_sound_start(it->second.get()) == MA_SUCCESS;
}

bool Audio::stop(SoundId id) {
  if (!impl_->engine_initialized || id == 0) {
    return false;
  }
  auto it = impl_->sounds.find(id);
  if (it == impl_->sounds.end()) {
    return false;
  }
  return ma_sound_stop(it->second.get()) == MA_SUCCESS;
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
  ma_sound_set_volume(it->second.get(), std::min(v, 1.0f));
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
}

} // namespace glintfx
