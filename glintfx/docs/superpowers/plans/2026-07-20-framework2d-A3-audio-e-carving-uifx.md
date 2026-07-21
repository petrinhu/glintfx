# Framework 2D — Slice A3 (audio module) + ui/fx carving / Fatia A3 (módulo de áudio) + carving ui/fx

- **Status:** Planned (Caetano/CTO, 2026-07-20) — execution by orchestrator fan-out (max 4 sonnet agents + adversarial reviewer)
- **Governs:** [[0015-framework-2d-atomized-architecture]] phases A3 (audio) and the fx-carving phase declared in ADR-0015 (c)
- **Branch:** `feat/framework2d`. **NO push, NO tag, NO merge** without explicit leader authorization.

## EN (concise summary)

Two slices, executed on `feat/framework2d`:

**Slice 1 — A3 audio (`GLINTFX_MODULE_AUDIO`).** Vendored miniaudio (single header, `third_party/miniaudio/`, MIT-0/public-domain dual license → NOTICE). New atom depending on **core only** (no GL, no window, no RmlUi — same independence class as i18n, which is the explicit model). Public boundary `glintfx::Audio` (`include/glintfx/audio.hpp`, pimpl, zero miniaudio types crossing). Minimal surface: `init(AudioConfig)/shutdown()` (deterministic RAII, idempotent, dtor calls shutdown), `load_sound(path)→SoundId` (synchronous full decode via `MA_SOUND_FLAG_DECODE` — this is what makes the audio callback allocation-free), `play/stop/set_volume(id)`, `set_master_volume`, `stop_all`. Formats: **WAV mandatory; FLAC+MP3 come free from miniaudio's built-in decoders and are enabled; OGG is NOT free** (miniaudio needs external stb_vorbis for it) → deferred, documented. `AudioConfig::null_backend=true` selects miniaudio's null backend for headless CI tests (no ALSA/Xvfb needed — pure unit tests placed OUTSIDE the GLFW/xvfb blocks in `tests/CMakeLists.txt`, mirroring the i18n suite at line ~2144). Hardening: non-finite/negative volume rejected (keep previous), every operation before `init()` or after `shutdown()` returns `false` (no UAF — this is the GusWorld pain being fixed), hostile/truncated WAV → `load_sound` returns 0, never crashes. OFF ⇒ TU and symbols vanish (`nm` proof), link deps (`Threads`, `dl`, `m`) vanish from the aggregate.

**Slice 2 — ui/fx carving (`GLINTFX_MODULE_FX`).** Honest boundary, declared up front: the RCSS effects glow/box-shadow/drop-shadow/blur/backdrop-blur/mask/gradients are implemented by **RmlUi's own upstream backend** (`RmlUi_Renderer_GL3.cpp`, compiled from the FetchContent tree) — carving those would mean patching a third-party file; deferred with low ROI. What IS glintfx-authored fx today, and what this slice carves completely: (1) the tint + ripple GLSL programs, their `CompileShader/RenderShader/ReleaseShader` dispatch (wrapper `GlintfxShaderHandle`) and the backdrop-capture machinery (`ArmBackdropCapture/EnsureBackdropCaptured`) currently fused inside `render_gl3.cpp`; (2) the three decorator TUs `decorator_polygon/image_tint/ripple.cpp`; (3) their registration in `bootstrap.cpp`. Mechanism: ui keeps a new seam header `src/fx_hook.hpp` (abstract `FxHook` + `CreateGl3FxHook()` factory declared there, defined in the fx TU); `Gl3RenderInterface` holds `std::unique_ptr<FxHook> fx_` (constructed under `#if GLINTFX_MODULE_FX`, null otherwise) and its shader overrides delegate to the hook; the entire wrapper/dispatch/capture code moves verbatim into `src/fx/effects_gl3.cpp` (new fx OBJECT lib). `GLINTFX_MODULE_FX=OFF` ⇒ lean-ui: no fx TU, no fx symbols, unknown decorators in RCSS are ignored by RmlUi (fail-high). Regression gate: the existing fx sanity tests (`polygon_sanity`, `polygon_gradient_sanity`, `image_tint_sanity`, `ripple_sanity`, `ripple_stress_sanity`) are **byte-unmodified** and must pass identically in the full preset before AND after the carve — they are the pixel-equivalence proof (hand-derived expected pixels). New `lean-ui` preset; fx tests CMake-gated on the flag.

Execution: 4 sonnet agents (S2 carve → S1 audio in two phases → S3 integration/CI/Windows job → S4 adversarial reviewer), single heavy build at a time in `/var/tmp` (`TMPDIR=/var/tmp`), Xvfb runs via `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ... -j1`. Local gates per slice: full ctest matrix + **cppcheck + clang-tidy replicated exactly from the `lint-and-scan` job** (v0.13.0 lesson) + ASan leg in the canonical `localhost/glintfx-ci:f42` container. Windows CI: new `.github/workflows/windows-atoms.yml` building ONLY the pure atoms (audio + i18n + stb_image compile) via a dedicated mini-harness — no RmlUi/Freetype/GL/GLFW on Windows; full Windows build stays deferred.

---

## PT — plano completo

### 0. Contexto e não-negociáveis

- Branch `feat/framework2d`; commits em Conventional Commits pt-br citando os IDs `A3-AUDIO` e `FX-CARVE-1` (adicionar as duas linhas ao `TODO.md`, status `🔍 Pendente verificação` ao entregar — nunca `✅` direto).
- **Nenhum agente pusha, tagueia ou mergeia.** Push só com ordem explícita do líder.
- Implementer ≠ reviewer. O review adversarial EXECUTA (mutation no RAII do áudio; prova visual no fx).
- 1 build pesado de glintfx por vez (OOM ~19Gi); builds em `/var/tmp` com `export TMPDIR=/var/tmp` (nunca `/tmp`, é tmpfs).
- Decisões já tomadas pelo líder (não reabrir): miniaudio vendorizado; audio depende SÓ de core; RAII/shutdown determinístico obrigatório; split ui/fx ratificado (ADR-0015 (c)).

### 1. Fatia 1 — A3-AUDIO: módulo `GLINTFX_MODULE_AUDIO`

#### 1.1 Vendoring do miniaudio

- Arquivo: `glintfx/third_party/miniaudio/miniaudio.h` — baixar da release estável mais recente da série 0.11.x do repo `mackron/miniaudio` (verificar a última tag em https://github.com/mackron/miniaudio/releases e **pinar explicitamente**: registrar tag + commit SHA + data em `glintfx/third_party/miniaudio/README.md`, no mesmo formato do `vendor/core/README.md`).
- Licença: dual **MIT No Attribution (MIT-0) / public domain (unlicense)** — nova entrada no `NOTICE` (raiz do repo), citando a versão pinada.
- TU de implementação: `glintfx/src/miniaudio_impl.c` com `#define MINIAUDIO_IMPLEMENTATION` + configuração mínima:
  - `#define MA_NO_ENCODING` (nunca gravamos áudio nesta fatia), `#define MA_NO_GENERATION` (sem waveform/noise API).
  - **NÃO** definir `MA_NO_FLAC`/`MA_NO_MP3`: WAV/FLAC/MP3 são os decoders embutidos (dr_wav/dr_flac/dr_mp3) e vêm de graça. **OGG NÃO vem de graça** (exigiria vendorizar stb_vorbis) → fora desta fatia, documentado em `docs/audio.md` como extensão futura.
  - Compilar este TU com `-w` (terceiro, mesmo idioma dos backends RmlUi via `set_source_files_properties`); `src/audio.cpp` (nosso) com `-Wall -Wextra`.

#### 1.2 Fronteira pública — `include/glintfx/audio.hpp`

Nenhum tipo miniaudio cruza o header (pimpl). Superfície da fatia mínima:

```cpp
namespace glintfx {

struct AudioConfig {
  // EN: true selects miniaudio's null backend (headless CI, no sound device).
  bool null_backend = false;
};

class Audio {
public:
  using SoundId = std::uint32_t;          // 0 == invalid

  Audio();                                 // does NOT touch the device
  ~Audio();                                // deterministic: calls shutdown()
  Audio(const Audio&) = delete;
  Audio& operator=(const Audio&) = delete;

  bool init(const AudioConfig& cfg = {});  // false on failure; false if already initialized
  void shutdown();                         // idempotent; releases sounds BEFORE engine/device
  bool is_initialized() const;

  SoundId load_sound(const char* path);    // sync full decode (WAV/FLAC/MP3); 0 on any failure
  bool play(SoundId id);                   // restarts from 0 if already playing
  bool stop(SoundId id);
  bool set_volume(SoundId id, float v);    // rejects !isfinite or v<0 (keeps previous); clamps to [0,1]
  void set_master_volume(float v);         // same validation contract
  void stop_all();
};

} // namespace glintfx
```

Contratos (todos testados):
- **RAII/ordem de teardown (a dor do GusWorld):** `shutdown()` faz `ma_sound_uninit` de TODOS os sons **antes** de `ma_engine_uninit` (que derruba device/context). Idempotente; dtor chama `shutdown()`; nenhum caminho vaza handle ALSA nem toca som após free (ASan/LSan é o gate).
- **Real-time safety estrutural:** `load_sound` usa `ma_sound_init_from_file` com `MA_SOUND_FLAG_DECODE` (decode completo, síncrono, no momento do load) — o audio thread só lê PCM da memória; **proibido** streaming, decode assíncrono ou callback customizado nesta fatia (não instalamos NENHUM callback próprio → nada nosso roda no audio thread; sem lock/malloc/throw nosso lá por construção).
- **Guardas de lifecycle:** toda operação antes de `init()` ou depois de `shutdown()` retorna `false`/`0` (sem UAF). `init()` duas vezes retorna `false` sem re-inicializar. `init()` após `shutdown()` é permitido (re-init limpo).
- **Hardening de input:** volume `!isfinite`/negativo → rejeita mantendo o anterior (mesmo idioma do `set_dp_ratio`); `load_sound(nullptr)` → 0; WAV truncado/hostil → 0, nunca aborta o host (disciplina do `decorator_polygon`/i18n).
- **Polifonia adiada (declarado):** 1 instância por `SoundId`; `play()` de um som já tocando reinicia do zero (`ma_sound_seek_to_pcm_frame(0)` + start). Overlap do mesmo SFX = fatia futura (`ma_sound_init_copy`), documentado em `docs/audio.md`.
- **Null backend:** `AudioConfig::null_backend=true` cria `ma_context` próprio com `ma_backend_null` e o injeta no `ma_engine_config` — é o modo dos testes de CI (headless, sem ALSA, sem Xvfb).
- Sem `update()` por frame: o `ma_engine` tem thread de device própria.

Armazenamento interno: `std::unordered_map<SoundId, std::unique_ptr<SoundSlot>>` (endereço do `ma_sound` estável — miniaudio guarda ponteiros); contador monotônico pra `SoundId` (nunca reusa id → id velho após unload/shutdown falha de forma segura).

#### 1.3 CMake / config

- `option(GLINTFX_MODULE_AUDIO ... ON)` — genuinamente OFF-able desde o dia 1, mesmo padrão/posição do bloco `GLINTFX_MODULE_I18N` (nenhum chamador interno: a lib nunca chama `glintfx::Audio`; só o consumidor).
- OBJECT lib `glintfx_audio` (`src/audio.cpp` + `src/miniaudio_impl.c`), `target_include_directories` PUBLIC (include/ + binary include/) e PRIVATE `third_party/miniaudio`; PRIVATE link: `Threads::Threads` (`find_package(Threads REQUIRED)` sob o if), `${CMAKE_DL_LIBS}`, `m` (miniaudio dlopen'a ALSA/Pulse em runtime — **zero** dep de link de sistema de áudio). `glintfx_instrument_coverage`; alias `glintfx::audio`; fold no agregado sob o mesmo if.
- `config.hpp.in`: `#cmakedefine01 GLINTFX_MODULE_AUDIO` (bloco espelhando o do I18N).
- **Sem preset `audio-only` ainda** (honestidade ADR-0015: exigiria `GLINTFX_MODULE_UI=OFF`, que só nasce com a carving madura; registrar a lacuna no comentário da option).

#### 1.4 Testes (puros, sem GL/Xvfb — fora dos blocos GLFW/xvfb do `tests/CMakeLists.txt`, modelo = bloco i18n ~linha 2144)

1. `tests/audio_lifecycle_sanity.cpp` — init(null)→shutdown; shutdown 2x; dtor sem shutdown explícito; ops pré-init e pós-shutdown retornam false/0; re-init pós-shutdown funciona; init 2x retorna false.
2. `tests/audio_playback_sanity.cpp` — gera um WAV mínimo EM CÓDIGO (header RIFF/fmt/data + ~100 amostras PCM16, escrito em arquivo temporário — sem fixture binária no repo); load→play→stop→volume; `set_volume(NaN/inf/-1)` rejeitado mantendo anterior; `SoundId` inválido → false; path inexistente → 0.
3. `tests/audio_hostile_sanity.cpp` — WAV truncado no meio do header, arquivo de lixo aleatório, arquivo vazio, `load_sound(nullptr)` → sempre 0, nunca crash (auditoria-dominó: mesma classe de robustez do i18n/polygon).

#### 1.5 Docs

- `docs/audio.md` novo (bilíngue): API, contrato de lifecycle, formatos (WAV/FLAC/MP3; OGG adiado + por quê), null backend, limitação de polifonia, real-time safety.
- `CHANGELOG.md` (Unreleased): entrada A3-AUDIO. `README.md`: linha na tabela de módulos, se houver.

### 2. Fatia 2 — FX-CARVE-1: carving ui/fx (`GLINTFX_MODULE_FX`)

#### 2.1 Fronteira honesta (a recomendação de escopo)

**O que esta fatia carva por completo (todo o fx AUTORAL do glintfx):**
- Em `render_gl3.cpp` (fundido hoje): GLSL do tint (`kGlintfxTint*ShaderSrc`) e do ripple (`kGlintfxRippleFragmentShaderSrc`), `CompileGlintfxStage`, `GlintfxTintShaderData`/`GlintfxRippleShaderData`/`GlintfxShaderHandle`, os 3 overrides `CompileShader/RenderShader/ReleaseShader`, `EnsureTintProgramCompiled`/`EnsureRippleProgramCompiled` + caches de uniform, e `ArmBackdropCapture`/`EnsureBackdropCaptured` + estado de captura (L1.22).
- Os 3 TUs de decorator (`decorator_polygon.cpp`, `decorator_image_tint.cpp`, `decorator_ripple.cpp`) — já separados, passam pro gate/OBJECT lib fx.
- O registro dos 3 instancers no `bootstrap.cpp` (linhas ~1047-1080 + membros do Impl + includes) — guardado por `#if GLINTFX_MODULE_FX`.

**O que NÃO é carvável nesta fatia (declarado, não disfarçado):** glow/box-shadow, drop-shadow, blur, backdrop-blur, mask e gradientes são features RCSS nativas do RmlUi, renderizadas pelos shaders do backend upstream `RmlUi_Renderer_GL3.cpp` (compilado direto da árvore FetchContent). `GLINTFX_MODULE_FX=OFF` **não remove esses shaders do binário** — removê-los exigiria patch no arquivo de terceiro (temos precedente de patch idempotente no pin, mas o ROI é baixo: são pequenos e o custo de manter o patch a cada bump do pin é real). Fica adiado como fatia futura opcional (`FX-CARVE-2`, só se um consumidor pedir binário mínimo). O doc-comment da option `GLINTFX_MODULE_FX` DECLARA esta ressalva — é a sucessora da "ressalva honesta" da ADR-0015 (c).

**Por que esta é a fatia mínima defensável (e não "interface + 1 efeito"):** tint e ripple compartilham o MESMO chokepoint de despacho (o trio de overrides + o wrapper `GlintfxShaderHandle` que embrulha TODO handle). Mover o chokepoint move os dois efeitos juntos por construção — extrair "só um" deixaria o wrapper meio-dentro/meio-fora, estado misto PIOR de revisar e com o dobro de janelas de regressão. O polygon não tem shader próprio (reusa gradiente do RmlUi via `CompiledShader` padrão) — pra ele o gate é só CMake+registro. Logo: a carving completa do fx autoral É o menor passo coeso. O que fica de fora (upstream shaders, mover arquivos pra `src/fx/`… ver 2.5) fica explicitamente listado.

#### 2.2 Mecanismo — o ponto de extensão

1. **`src/fx_hook.hpp`** (novo, POSSE DA UI — header interno, pode usar tipos Rml, mesma convenção dos headers de src/):
```cpp
namespace glintfx {
class FxHook {
public:
  virtual ~FxHook() = default;
  // EN: full takeover of the shader pipeline: recognizes glintfx effect names,
  //     wraps+delegates every other name to ri's base class.
  virtual Rml::CompiledShaderHandle compile_shader(RenderInterface_GL3& ri,
      const Rml::String& name, const Rml::Dictionary& params) = 0;
  virtual void render_shader(RenderInterface_GL3& ri, Rml::CompiledShaderHandle h,
      Rml::CompiledGeometryHandle g, Rml::Vector2f translation, Rml::TextureHandle t) = 0;
  virtual void release_shader(RenderInterface_GL3& ri, Rml::CompiledShaderHandle h) = 0;
  virtual void arm_backdrop_capture(int offset_x, int offset_y, int w, int h) = 0;
};
// EN: defined in src/fx/effects_gl3.cpp (fx module); only referenced under
//     #if GLINTFX_MODULE_FX, so fx=OFF never creates an undefined symbol.
std::unique_ptr<FxHook> CreateGl3FxHook();
}
```
2. **`src/fx/effects_gl3.cpp`** (novo, módulo fx): classe concreta `Gl3FxHook : FxHook` — recebe TODO o código listado em 2.1 movido **verbatim** (cut-paste com comentários preservados; mudanças só onde a costura exige):
   - `compile_shader`: lógica atual dos ramos tint/ripple/else-wrap-base (o wrapper `GlintfxShaderHandle` vive AQUI agora).
   - `render_shader`: unwrap + draw (tint/ripple) ou delegação de base via **chamada qualificada** `ri.RenderInterface_GL3::RenderShader(wrapper->base_handle, g, translation, t)` (válida de fora da classe, suprime despacho virtual; `GetTransform()`/`ResetProgram()` são públicos — ADR-0010 (a) continua satisfeita: o draw roda com acesso a eles via `ri`).
   - `release_shader`: idem, `ri.RenderInterface_GL3::ReleaseShader(...)` no ramo base.
   - `arm_backdrop_capture` + `EnsureBackdropCaptured` + textura/estado de captura como membros do hook. O dtor do hook libera `tint_program_`/`ripple_program_`/`backdrop_capture_tex_` — timing idêntico ao atual porque o hook é membro `unique_ptr` do `Gl3RenderInterface` (destruído junto, com contexto GL vivo).
3. **`render_gl3.cpp` (ui, emagrece):** mantém `LoadTexture` (concern de image, fica) e ganha `std::unique_ptr<FxHook> fx_;` — construído no ctor sob `#if GLINTFX_MODULE_FX` (`fx_ = CreateGl3FxHook();`), senão null. Overrides viram:
   `if (fx_) return fx_->compile_shader(*this, name, params); return RenderInterface_GL3::CompileShader(name, params);` (idem render/release). `begin_frame_compose`: `if (impl_->renderer.fx()) ...arm...` — na prática, método `Gl3RenderInterface::arm_backdrop(...)` que faz `if (fx_) fx_->arm_backdrop_capture(...)`. Adicionar `#include <glintfx/config.hpp>`. Único `#ifdef` novo no arquivo: a construção do hook.
   Invariante de consistência por-build: com fx ON, TODO handle em voo é wrapper; com OFF, todo handle é cru da base — nunca misto.
4. **`bootstrap.cpp`:** includes dos 3 decorators, membros `polygon_instancer/tint_instancer/ripple_instancer` do Impl e os 3 blocos de registro sob `#if GLINTFX_MODULE_FX` (já inclui config.hpp pro OWN_FONT_ENGINE). Com OFF, `decorator: polygon(...)` no RCSS não resolve → RmlUi loga e ignora (fail-high coerente com a casa).

#### 2.3 CMake / presets

- `option(GLINTFX_MODULE_FX ... ON)` no bloco ADR-0015, substituindo a nota "ui/fx: NOT introduced in A0" (atualizar o comentário: fx nasce AGORA porque o gate passou a existir; ui continua sem flag — segue sendo o produto inteiro, a flag dela nasce quando houver composição sem ui).
- Remover os 3 `decorator_*.cpp` de `_glintfx_sources`; OBJECT lib `glintfx_fx` sob o if: sources `src/fx/effects_gl3.cpp` + os 3 decorators; PRIVATE: `RmlUi::Core`, `glloader`, include dirs (public + binary + `${_rml_backends}` pro `RmlUi_Renderer_GL3.h`); `-Wall -Wextra`; coverage; alias `glintfx::fx`; fold no agregado sob o if.
- `config.hpp.in`: `#cmakedefine01 GLINTFX_MODULE_FX`.
- `CMakePresets.json`: preset **`lean-ui`** (`GLINTFX_MODULE_FX=OFF`, `GLINTFX_BUILD_TESTS=ON`, clang, `binaryDir build-lean-ui`) — a composição que a ADR-0015 (c)/(d)(5) existe pra servir.
- `tests/CMakeLists.txt`: os alvos `polygon_sanity`, `polygon_gradient_sanity`, `image_tint_sanity`, `ripple_sanity`, `ripple_stress_sanity` (e os configure_file das cenas deles) entram num bloco `if(GLINTFX_MODULE_FX)`.

#### 2.4 Prova de regressão (obrigatória, na ordem)

1. **Baseline ANTES de tocar em qualquer source:** build full no HEAD atual + suíte inteira verde (registrar o log). Os testes de fx têm pixels esperados derivados à mão — eles SÃO a prova pixel-equivalente.
2. **Regra de ouro:** os `.cpp` de teste de fx são **byte-unmodified** nesta fatia (só o gating no tests/CMakeLists.txt muda). "Mesmos testes, mesmos pixels, zero edição" é o que dá sentido à prova.
3. **Depois da carve:** (a) full preset → suíte INTEIRA verde (incl. os 5 de fx + `render_sanity` estatístico); (b) `lean-ui` → configura, compila, linka, suíte-menos-fx verde + prova de símbolo: `nm build-lean-ui/libglintfx.a | grep -E "Ripple|ImageTint|PolygonDecorator|Gl3FxHook"` → 0 matches (e no full, >0); (c) embed-only (GLFW=OFF, fx ON — a composição do GusWorld) → suíte embed verde.
4. **Atenção `render_sanity`/showcase em lean-ui:** grep `showcase_test.rml`/cenas compartilhadas por `polygon|image-tint|ripple`. Se a cena usa decorator autoral, no lean-ui o decorator é ignorado e as estatísticas de pixel podem mudar → prover variante `showcase_test_lean.rml` (ou gate das asserções) SÓ se a suíte lean falhar; verificar empiricamente antes de mexer.
5. ASan leg (container f42) na configuração full — o wrapper `GlintfxShaderHandle` mudou de dono; a janela clássica de double-free/UAF do ADR-0010 (a) precisa do gate de memória.

#### 2.5 Fica para depois (explícito)

- `FX-CARVE-2` (opcional, sob demanda): patch no backend upstream pra remover shaders de filtro RmlUi do binário lean-ui.
- Relocação cosmética dos `decorator_*.{cpp,hpp}` pra `src/fx/` (git mv puro; fora desta fatia pra manter o diff revisável = só a extração real de lógica).
- `GLINTFX_MODULE_UI` como flag real (nasce quando existir composição sem ui — ex.: audio-only de verdade).
- Preset `audio-only` (depende do item acima).

### 3. Topologia de execução (orquestrador)

**Regra de colisão:** `glintfx/CMakeLists.txt`, `tests/CMakeLists.txt`, `config.hpp.in`, `CMakePresets.json`, `CHANGELOG.md`, `TODO.md`, `NOTICE` são tocados pelas duas fatias → a edição desses arquivos é SERIALIZADA por posse exclusiva por onda (nunca dois agentes no mesmo arquivo).

| Onda | Agente | Papel | Roda em paralelo com | Build? |
|---|---|---|---|---|
| 1 | **S2** `engine-graphics-programmer` | Carving completa (sources + CMake + presets + gating de testes) — dono de TODOS os arquivos compartilhados na onda 1 | S1 fase A | SIM (slot único) |
| 1 | **S1** `backend-engineer` | Áudio fase A: vendoring + sources + testes + docs (SEM tocar CMake/config/NOTICE/CHANGELOG/TODO; entrega os blocos prontos no relatório) | S2 | NÃO |
| 2 | **S1 (continuação)** | Áudio fase B: aplica os blocos de CMake/config/NOTICE/etc. por cima da árvore pós-carve; builda e testa | — | SIM |
| 3 | **S3** `devops-sre` | Integração de CI: job Windows (`windows-atoms.yml`, arquivo NOVO), leg ASan f42 das duas fatias, matrix de verificação final, lint | — | SIM (serial) |
| 4 | **S4** `qa-engineer` | Review adversarial que executa (seção 6) | — | reusa builds |

Carve primeiro porque reestrutura o CMakeLists (fx OBJECT lib + remoção de sources da lista plana); o áudio aplica blocos aditivos por cima — menor superfície de conflito. O orquestrador re-verifica build limpo + spot-check das claims arquivo:linha de CADA agente antes de aceitar (relatório de agent não é prova).

### 4. Comandos canônicos (todo agente usa exatamente estes)

```sh
export TMPDIR=/var/tmp
mkdir -p /var/tmp/fake_xdg_runtime && chmod 700 /var/tmp/fake_xdg_runtime

# configure+build (1 leg por vez; nomes de build dir por leg em /var/tmp)
cmake -S glintfx -B /var/tmp/glintfx-<leg> -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DGLINTFX_BUILD_TESTS=ON [flags da leg]
cmake --build /var/tmp/glintfx-<leg> -j$(nproc)

# suíte (Xvfb; -j1 sempre)
env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime \
  xvfb-run -a ctest --test-dir /var/tmp/glintfx-<leg> --output-on-failure -j1

# lint (OBRIGATÓRIO por fatia -- lição v0.13.0; replicar EXATAMENTE o job lint-and-scan
# de .github/workflows/ci.yml, inclusive flags/file-filter/--error-exitcode=1):
cmake -S glintfx -B /var/tmp/glintfx-lint -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DGLINTFX_BUILD_TESTS=ON
clang-tidy -p /var/tmp/glintfx-lint glintfx/src/*.cpp        # gate duro: clang-analyzer-*
# cppcheck: copiar a invocação literal do job (enable=warning,style,performance,portability,
# --error-exitcode=1, --project=compile_commands.json com --file-filter de src/) -- LER o yml.

# ASan no container canônico f42 (1 por vez; imagem: tools/ci/build_image.sh se faltar)
podman run --rm -v "$PWD":/src -w /src localhost/glintfx-ci:f42 bash -lc '
  cmake -S glintfx -B /var/tmp/b-asan -DGLINTFX_BUILD_TESTS=ON -DGLINTFX_SANITIZE=ON \
        -DCMAKE_BUILD_TYPE=Debug &&
  cmake --build /var/tmp/b-asan -j$(nproc) &&
  xvfb-run -a ctest --test-dir /var/tmp/b-asan --output-on-failure'
```

Legs de verificação final (S3, nesta ordem, uma por vez): full · lean-ui (`-DGLINTFX_MODULE_FX=OFF`) · audio-off (`-DGLINTFX_MODULE_AUDIO=OFF`) · embed-only (`-DGLINTFX_MODULE_WINDOW=OFF`) · ASan-full (container f42). Ao final: `gio trash`/limpeza dos build dirs de `/var/tmp` que não forem mais necessários (disco real, mas não acumular dezenas de GB).

### 5. CI Windows (pragmático, Linux-first preservado)

Arquivo NOVO `.github/workflows/windows-atoms.yml` (não tocar no `ci.yml`):

- **Escopo:** valida SÓ os átomos puros — audio (miniaudio→WASAPI compila; testes rodam com null backend), i18n (std-lib pura) e image (compile-only do `stb_image_impl.cpp`). **Nada** de RmlUi/Freetype/GLFW/GL/Xvfb no Windows.
- **Mecanismo:** mini-harness dedicado `glintfx/tests/win-atoms/CMakeLists.txt` — projeto CMake standalone que compila `src/i18n.cpp`, `src/audio.cpp`, `src/miniaudio_impl.c`, `src/stb_image_impl.cpp` + os testes `i18n_*.cpp` e `audio_*.cpp` contra `include/` (e `third_party/{miniaudio,stb}`). Sem FetchContent, sem find_package além de Threads. Isso evita o custo/risco de portar o build inteiro (RmlUi+Freetype via vcpkg ≈ >10 min/job + manutenção) — adiado, declarado.
- **Job:** `runs-on: windows-latest`, MSVC default; `cmake -S glintfx/tests/win-atoms -B build && cmake --build build --config Release && ctest --test-dir build -C Release --output-on-failure`. Triggers iguais ao ci.yml (push/PR). Custo ~2-3 min. Risco baixo: miniaudio é first-class no MSVC; i18n é std-lib; o maior risco é warning/strictness MSVC em `audio.cpp` (nosso, consertável). Se algo do harness travar >2 iterações, degradar para compile-only (sem ctest) e reportar — não segurar a onda.
- Gate: bloqueante por ser barato; se demonstrar flakiness, o líder decide rebaixar.

### 6. Review adversarial (S4 — executa, não lê)

1. **RAII do áudio (mutation testing):** (a) mutar `shutdown()` removendo o loop de `ma_sound_uninit` → ASan/LSan DEVE ficar vermelho nos testes de lifecycle (se ficar verde, a suíte é fraca → achado); (b) mutar a ordem (engine antes dos sounds) → deve falhar; (c) reverter mutações, rodar `audio_*` sob ASan 20x seguidas (flakiness); (d) tentar UAF pela API: `load`→`shutdown`→`play(id)` e `Audio` destruído com som tocando.
2. **Hostile input áudio:** fuzz leve manual — WAV com `data` chunk maior que o arquivo, riff size 0xFFFFFFFF, arquivo de 1 byte; `set_volume(NaN)` etc. Nenhum crash/abort.
3. **fx regressão visual:** rodar os 5 testes de fx no full ANTES (baseline em stash/worktree do HEAD pré-carve) e DEPOIS — mesmos resultados; confirmar `git diff` vazio nos `.cpp` de teste fx; nm-proof do lean-ui (seção 2.4.3b) executado de verdade; embed-only verde.
4. **Fronteiras:** `tools/check_encapsulation.sh` + grep de `#include` em `include/glintfx/audio.hpp` (zero miniaudio/terceiro); fx→ui de mão única (ui não inclui nada de `src/fx/` além do seam `fx_hook.hpp`, que é da ui).
5. **Lint:** re-rodar cppcheck + clang-tidy do zero (não confiar no log dos implementers).
6. Severidade nos achados (CRÍTICO/IMPORTANTE/COSMÉTICO); CRÍTICO bloqueia o `🔍`.

### 7. Riscos

| Risco | Prob. | Mitigação |
|---|---|---|
| Regressão visual na carve (dono do wrapper mudou) | média | baseline-antes, testes fx byte-unmodified, ASan f42, review que executa |
| `render_sanity`/showcase quebrar em lean-ui (cena usa decorator autoral) | média | passo 2.4.4 (verificar empiricamente; variante lean só se necessário) |
| OOM (2 builds glintfx simultâneos) | alta se violado | serialização explícita (topologia seção 3); builds em /var/tmp |
| miniaudio + MSVC/warnings ou ALSA runtime no f42 | baixa | null backend nos testes; `-w` no TU de terceiro; degradar Windows p/ compile-only |
| Conflito nos arquivos compartilhados | alta se violado | posse exclusiva por onda (S2 → S1-faseB → S3) |
| cppcheck-style derrubar CI de novo (lição v0.13.0) | média | lint local obrigatório por fatia, invocação idêntica ao yml |
| Timing de destruição do hook fx sem contexto GL | baixa | hook é membro do Gl3RenderInterface (timing idêntico ao atual); reviewer valida teardown sob ASan |

### 8. Perguntas em aberto (não bloqueiam; decididas por engenharia, confirmar retroativamente)

1. Clamp de volume em `[0,1]` (sem amplificação >1) — escolhido por segurança de ouvido/consistência; trivial de relaxar depois.
2. FLAC/MP3 habilitados por vir de graça no miniaudio (superfície de decode maior que "só WAV"); se o líder preferir binário/superfície mínimos: `MA_NO_FLAC`+`MA_NO_MP3` é 1 linha cada.
3. Job Windows bloqueante vs informativo — proposto bloqueante por ser barato; rebaixável.
