# glintfx/third_party/miniaudio -- vendored miniaudio single-header library (EN)

This directory vendors [miniaudio](https://github.com/mackron/miniaudio) by David Reid, a
single-header audio playback/capture library, for the `GLINTFX_MODULE_AUDIO` atom
(`A3-AUDIO`, ADR-0015 (b) -- "audio depends on core ONLY", same independence class as `i18n`).

## Pinned version

| Field | Value |
|---|---|
| Tag | `0.11.25` (latest stable in the `0.11.x` series at pin time) |
| Commit SHA | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` |
| Commit date | 2026-03-04 |
| Pinned on | 2026-07-21 |
| Source | `https://raw.githubusercontent.com/mackron/miniaudio/9634bedb5b5a2ca38c1ee7108a9358a4e233f14d/miniaudio.h` |
| SHA-256 of vendored file | `ac7af4de748b7e26b777f37e01cee313a308a7296a3eb080e2906b320cc55c89` |

Fetched via the exact tag commit (not `master`), byte-identical to upstream at fetch time.
Bumping the pin means re-downloading a newer tagged commit, updating this table, and
re-checking the patch below (it may already be fixed upstream, in which case drop it).

## Patch applied on top of the pin (not upstream -- disclosed)

One line changed in `ma_resource_manager_data_buffer_node_acquire()` (search the file for
`[glintfx PATCH, A3-AUDIO, not upstream]` -- the change and its full rationale are documented
inline at the exact site): a missing `result == MA_SUCCESS &&` guard caused this function to
unconditionally dereference `pDataBufferNode` in its epilogue even on the branch where that
same pointer had just been `ma_free()`'d a few lines above -- a genuine heap-use-after-free,
caught live under AddressSanitizer while this module's own test suite (`audio_hostile_sanity`,
`audio_playback_sanity`) called `ma_sound_init_from_file()` (glintfx's `load_sound()`) on a
nonexistent/hostile file synchronously. Reproduced independently of glintfx-authored code with
a minimal standalone repro against this exact pinned commit. Same disclosure discipline as
`glintfx/patches/README.md`'s RmlUi patch (that one is applied via CMake's `PATCH_COMMAND`
because RmlUi is `FetchContent`-fetched; miniaudio is instead directly vendored as a single
file, so its one-line fix lives inline in the committed copy, marked at the site, rather than
as a separate `.patch` file). Intended to be reported upstream and dropped once fixed there and
the pin is bumped past it (reporting is an outward-facing action -- left to the líder's
authorization, not done unilaterally from this slice).

## License

Dual-licensed by upstream, our choice (see the license block at the end of `miniaudio.h`):
**MIT No Attribution (MIT-0)** or **public domain (unlicense.org)**. We list it in the
repository's `NOTICE` as "MIT-0 / public domain". One line of the vendored file is patched, see
above -- both licenses permit modification.

## What links against it

- `glintfx/src/miniaudio_impl.c`: the single translation unit that defines
  `MINIAUDIO_IMPLEMENTATION` (see that file's header comment for the exact feature-macro
  configuration: `MA_NO_ENCODING` + `MA_NO_GENERATION`, WAV/FLAC/MP3 decoders enabled, OGG
  deferred).
- `glintfx/src/audio.cpp`: `glintfx::Audio`, the pimpl wrapper that is the module's entire
  public surface (`glintfx/include/glintfx/audio.hpp`); no miniaudio type crosses that header.

Gated by CMake option `GLINTFX_MODULE_AUDIO` (OBJECT library `glintfx_audio`,
`glintfx/CMakeLists.txt`); `OFF` removes both the translation units and their symbols.

---

# glintfx/third_party/miniaudio -- biblioteca vendorizada miniaudio, single-header (PT)

Este diretório vendoriza o [miniaudio](https://github.com/mackron/miniaudio), de David Reid,
uma biblioteca single-header de reprodução/captura de áudio, para o átomo
`GLINTFX_MODULE_AUDIO` (`A3-AUDIO`, ADR-0015 (b): "audio depende SÓ de core", mesma classe de
independência do `i18n`).

## Versão pinada

| Campo | Valor |
|---|---|
| Tag | `0.11.25` (última estável da série `0.11.x` no momento do pin) |
| Commit SHA | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` |
| Data do commit | 2026-03-04 |
| Pinado em | 2026-07-21 |
| Origem | `https://raw.githubusercontent.com/mackron/miniaudio/9634bedb5b5a2ca38c1ee7108a9358a4e233f14d/miniaudio.h` |
| SHA-256 do arquivo vendorizado | `ac7af4de748b7e26b777f37e01cee313a308a7296a3eb080e2906b320cc55c89` |

Baixado exatamente do commit da tag (não do `master`), byte-idêntico ao upstream no momento do
fetch. Atualizar o pin significa baixar de novo um commit de tag mais recente, atualizar esta
tabela, e reverificar o patch abaixo (ele pode já estar corrigido no upstream, nesse caso
remova-o).

## Patch aplicado sobre o pin (não é upstream -- divulgado)

Uma linha alterada em `ma_resource_manager_data_buffer_node_acquire()` (procure no arquivo por
`[glintfx PATCH, A3-AUDIO, not upstream]` -- a mudança e a racional completa estão documentadas
inline no próprio local): faltava uma guarda `result == MA_SUCCESS &&` que fazia esta função
desreferenciar `pDataBufferNode` incondicionalmente no epílogo, mesmo no ramo onde esse mesmo
ponteiro tinha acabado de ser `ma_free()`'d algumas linhas acima -- um heap-use-after-free
genuíno, capturado ao vivo sob AddressSanitizer enquanto a própria suíte de testes deste módulo
(`audio_hostile_sanity`, `audio_playback_sanity`) chamava `ma_sound_init_from_file()` (o
`load_sound()` do glintfx) num arquivo inexistente/hostil de forma síncrona. Reproduzido de
forma independente do código autoral do glintfx, com um repro standalone mínimo contra este
exato commit pinado. Mesma disciplina de divulgação do patch RmlUi em
`glintfx/patches/README.md` (aquele é aplicado via `PATCH_COMMAND` do CMake porque o RmlUi é
fetchado via `FetchContent`; o miniaudio, em vez disso, é vendorizado diretamente como um único
arquivo, então a correção de uma linha vive inline na cópia commitada, marcada no local, em vez
de um arquivo `.patch` separado). Pensado para ser reportado ao upstream e removido assim que
corrigido lá e o pin avançar para além dele (reportar é uma ação voltada pra fora -- deixado
para a autorização do líder, não feito unilateralmente a partir desta fatia).

## Licença

Dual-licenciado pelo upstream, escolha nossa (ver o bloco de licença no final do
`miniaudio.h`): **MIT No Attribution (MIT-0)** ou **domínio público (unlicense.org)**.
Listado no `NOTICE` do repositório como "MIT-0 / domínio público". Uma linha do arquivo
vendorizado é patcheada, ver acima -- as duas licenças permitem modificação.

## O que linka contra ele

- `glintfx/src/miniaudio_impl.c`: a única unidade de tradução que define
  `MINIAUDIO_IMPLEMENTATION` (ver o comentário de cabeçalho desse arquivo pela configuração
  exata de feature-macros: `MA_NO_ENCODING` + `MA_NO_GENERATION`, decoders WAV/FLAC/MP3
  habilitados, OGG adiado).
- `glintfx/src/audio.cpp`: `glintfx::Audio`, o wrapper pimpl que é a superfície pública
  inteira do módulo (`glintfx/include/glintfx/audio.hpp`); nenhum tipo miniaudio cruza esse
  header.

Gateado pela opção CMake `GLINTFX_MODULE_AUDIO` (biblioteca OBJECT `glintfx_audio`,
`glintfx/CMakeLists.txt`); `OFF` remove tanto as unidades de tradução quanto os símbolos delas.
