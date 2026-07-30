// SPDX-License-Identifier: MPL-2.0
// EN: IMG-ENCODE (W21, 2026-07-30) -- implementation of the public `glintfx/image.hpp` encode
//     pair (`encode_image_memory()`/`encode_image_file()`), the symmetric counterpart to
//     `image.cpp`'s decode pair. See `glintfx/image.hpp`'s own top comment (right above the
//     `ImageFormat` enum) for the full per-format contract, the pixel-layout convention, and the
//     complete fail-high guard list this file implements -- this file's own comments below focus
//     on the IMPLEMENTATION choices that comment does not already cover.
//
//     DISPATCH SHAPE: `encode_image_memory()` is the ONE place every format's actual encode call
//     happens; `encode_image_file()` is a thin wrapper that calls it once and streams the
//     resulting buffer to disk -- the same "file entry point delegates to the memory entry
//     point" shape `image.cpp`'s `decode_image_file()`/`decode_image_memory()` use in the
//     opposite direction. Five of the seven formats (PNG/JPG/BMP/TGA/HDR) go through
//     stb_image_write's own `_to_func` callback API (`write_cb` below appends every chunk it is
//     handed to `EncodedImageBytes::bytes`) rather than its filename-taking overloads -- see
//     `stb_image_write_impl.cpp`'s own top comment for why (`STBI_WRITE_NO_STDIO`, avoids a
//     second vendored file-I/O path this module never needs). PPM (no existing encoder to call --
//     this module writes the trivial P6 header + raw RGB bytes itself) and QOI (this repository's
//     own clean-room encoder, `qoi_encode.hpp`) are the two formats with no stb_image_write
//     counterpart.
//
//     kMaxImageEncodeBytes DERIVATION: `4096 * 4096 * 4` bytes, exactly 64 MiB -- picked, not
//     copied from the DECODE side's own `kMaxImageDecodeBytes` (256 MiB, `image_decode.hpp`),
//     because the two caps guard fundamentally different things: the decode cap bounds an
//     ENCODED (typically compressed) byte buffer BEFORE any decompression happens, so 256 MiB of
//     compressed input can expand far past 256 MiB of raw pixels once decoded; the encode cap
//     here bounds the RAW, ALREADY-UNCOMPRESSED `width * height * 4` pixel buffer a caller hands
//     in -- there is no compression ratio to borrow headroom from. 64 MiB was chosen so a 4K UHD
//     RGBA8 frame (`3840 * 2160 * 4` = 33,177,600 bytes ≈ 31.6 MiB) fits with roughly 2x headroom
//     -- generous for this API's own named use cases (a screen-capture buffer, a frozen
//     UI-background composite), both realistically at-or-below display resolution -- while the
//     boundary itself lands on a round, easy-to-state number (`4096 * 4096`, a plausible texture-
//     atlas-sized square) that a test can afford to allocate for REAL at the "exactly at the cap,
//     must still succeed" boundary case (see `tests/image_encode_sanity.cpp`'s own boundary
//     group, and this repository's own canonized lesson that a boundary test only proves the
//     boundary if it attacks the EDGE with real buffers, not an absurd value that never actually
//     runs the real code path). An 8K capture (`7680 * 4320 * 4` ≈ 132.7 MiB) does NOT fit under
//     this cap -- a DECLARED ceiling, not an oversight; see `docs/capabilities.md`'s Image-encode
//     row for the same limit stated as a consumer-facing promise, and this file's own note that
//     raising it later (if an 8K-capture consumer need is ever measured) is a one-line change
//     with no API-shape impact.
//
//     FILE-WRITE FAILURE LEAVES NO PARTIAL FILE: `encode_image_file()` opens with
//     `std::ios::trunc` (any pre-existing file at `path` is replaced, not appended to -- matches
//     every other single-shot file-write call in this library, e.g. `App::snapshot()`'s own PPM
//     write, `app.cpp`) and, on a short/failed `write()`, calls `std::remove(path)` before
//     returning `false` -- a caller checking the return value never has to distinguish "no file
//     was written" from "a truncated, corrupt file was written and left behind"; both degrade to
//     the SAME observable state (no valid file at `path`). `std::remove()`'s own return value is
//     deliberately IGNORED here: if the removal itself fails (e.g. the filesystem went read-only
//     mid-write, an exotic but real failure mode), there is nothing more this function can safely
//     do about it, and the caller already received `false` -- the same "cannot degrade further,
//     the false already told the truth" reasoning `glintfx::log()`'s own built-in default sink
//     uses for an `fprintf` it does not itself verify.
// PT: IMG-ENCODE (W21, 2026-07-30) -- implementação do par público de encode de
//     `glintfx/image.hpp` (`encode_image_memory()`/`encode_image_file()`), a contraparte
//     simétrica do par de decode de `image.cpp`. Ver o próprio comentário de topo de
//     `glintfx/image.hpp` (logo acima do enum `ImageFormat`) pro contrato completo formato-a-
//     formato, a convenção de layout de pixel, e a lista de guarda fail-high completa que este
//     arquivo implementa -- os próprios comentários deste arquivo abaixo focam nas escolhas de
//     IMPLEMENTAÇÃO que aquele comentário ainda não cobre.
//
//     FORMA DO DISPATCH: `encode_image_memory()` é o ÚNICO lugar onde a chamada de encode real de
//     todo formato acontece; `encode_image_file()` é um wrapper fino que a chama uma vez e
//     transmite o buffer resultante pro disco -- a mesma forma "ponto de entrada de arquivo
//     delega pro ponto de entrada de memória" que `decode_image_file()`/`decode_image_memory()`
//     de `image.cpp` usam na direção oposta. Cinco dos sete formatos (PNG/JPG/BMP/TGA/HDR) passam
//     pela própria API de callback `_to_func` do stb_image_write (`write_cb` abaixo acrescenta
//     todo pedaço que recebe a `EncodedImageBytes::bytes`) em vez dos overloads que tomam nome de
//     arquivo -- ver o próprio comentário de topo de `stb_image_write_impl.cpp` pro porquê
//     (`STBI_WRITE_NO_STDIO`, evita um segundo caminho de I/O de arquivo vendorizado que este
//     módulo nunca precisa). PPM (sem encoder existente pra chamar -- este módulo escreve o
//     cabeçalho P6 trivial + bytes RGB crus ele mesmo) e QOI (o próprio encoder clean-room deste
//     repositório, `qoi_encode.hpp`) são os dois formatos sem contraparte no stb_image_write.
//
//     DERIVAÇÃO DE kMaxImageEncodeBytes: `4096 * 4096 * 4` bytes, exatamente 64 MiB -- escolhido,
//     não copiado do próprio `kMaxImageDecodeBytes` do lado DECODE (256 MiB, `image_decode.hpp`),
//     porque os dois tetos guardam coisas fundamentalmente diferentes: o teto de decode limita um
//     buffer de bytes CODIFICADO (tipicamente comprimido) ANTES de qualquer descompressão
//     acontecer, então 256 MiB de input comprimido pode expandir bem além de 256 MiB de pixels
//     crus uma vez decodificado; o teto de encode aqui limita o buffer de pixel `width * height *
//     4` CRU, JÁ-DESCOMPRIMIDO que um chamador entrega -- não há proporção de compressão nenhuma
//     pra emprestar folga. 64 MiB foi escolhido pra que um frame 4K UHD RGBA8 (`3840 * 2160 * 4` =
//     33.177.600 bytes ≈ 31,6 MiB) caiba com cerca de 2x de folga -- generoso pros próprios casos
//     de uso nomeados desta API (um buffer de captura de tela, uma composição de fundo de UI
//     congelado), ambos realisticamente na ou abaixo da resolução de display -- enquanto a
//     própria fronteira cai num número redondo, fácil de dizer (`4096 * 4096`, um tamanho
//     plausível de atlas de textura quadrado) que um teste consegue se dar ao luxo de alocar DE
//     VERDADE no caso de fronteira "exatamente no teto, ainda precisa ter sucesso" (ver o próprio
//     grupo de fronteira de `tests/image_encode_sanity.cpp`, e a própria lição canonizada deste
//     repositório de que um teste de fronteira só prova a fronteira se atacar a BEIRA com buffers
//     reais, não um valor absurdo que nunca de fato roda o caminho de código real). Uma captura 8K
//     (`7680 * 4320 * 4` ≈ 132,7 MiB) NÃO cabe sob este teto -- um teto DECLARADO, não um
//     descuido; ver a linha de encode-de-imagem de `docs/capabilities.md` pro mesmo limite dito
//     como promessa voltada ao consumidor, e a própria nota deste arquivo de que elevá-lo depois
//     (se uma necessidade de consumidor de captura 8K for algum dia medida) é uma mudança de uma
//     linha só, sem impacto na forma da API.
//
//     FALHA DE ESCRITA DE ARQUIVO NÃO DEIXA ARQUIVO PARCIAL: `encode_image_file()` abre com
//     `std::ios::trunc` (um arquivo pré-existente em `path` é substituído, não anexado -- bate
//     com toda outra chamada de escrita de arquivo de um-tiro-só desta biblioteca, ex.: a própria
//     escrita de PPM do `App::snapshot()`, `app.cpp`) e, numa `write()` curta/falha, chama
//     `std::remove(path)` antes de devolver `false` -- um chamador checando o valor de retorno
//     nunca precisa distinguir "nenhum arquivo foi escrito" de "um arquivo truncado e corrompido
//     foi escrito e deixado pra trás"; os dois degradam pro MESMO estado observável (nenhum
//     arquivo válido em `path`). O próprio valor de retorno de `std::remove()` é deliberadamente
//     IGNORADO aqui: se a própria remoção falhar (ex.: o filesystem ficou somente-leitura no meio
//     da escrita, um modo de falha exótico mas real), não há mais nada que esta função possa
//     fazer com segurança a respeito, e o chamador já recebeu `false` -- o mesmo raciocínio "não
//     dá pra degradar mais, o false já disse a verdade" que o próprio sink default embutido de
//     `glintfx::log()` usa pro `fprintf` que ele mesmo não verifica.
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

// EN: NO `STBI_WRITE_NO_STDIO` define here (deliberately -- this TU only ever calls the
//     `_to_func` callback variants below, never a filename-taking overload) -- see
//     `stb_image_write_impl.cpp`'s own top comment for the measured upstream quirk that makes
//     defining it here actively WRONG: it silently deletes `stbi_write_hdr_to_func()`, the
//     `ImageFormat::Hdr` case below needs, from the implementation TU (an unrelated
//     `#ifndef STBI_WRITE_NO_STDIO` block in the vendored header accidentally spans the entire
//     HDR writer section, not just the stdio-only overloads it is meant to gate).
// PT: SEM define `STBI_WRITE_NO_STDIO` aqui (deliberadamente -- esta TU só chama as variantes de
//     callback `_to_func` abaixo, nunca um overload baseado em nome de arquivo) -- ver o próprio
//     comentário de topo de `stb_image_write_impl.cpp` pra peculiaridade upstream medida que faz
//     definir isto aqui ativamente ERRADO: apaga silenciosamente `stbi_write_hdr_to_func()`, que
//     o caso `ImageFormat::Hdr` abaixo precisa, da TU de implementação (um bloco
//     `#ifndef STBI_WRITE_NO_STDIO` não-relacionado no header vendorizado acidentalmente
//     abrange a seção inteira do escritor HDR, não só os overloads só-stdio que deveria gatear).
#include "stb_image_write.h"

#include "qoi_encode.hpp"

#include <cstdio>
#include <fstream>
#include <ios>
#include <limits>
#include <vector>

namespace glintfx {

namespace {

// EN: The 64 MiB raw-pixel-buffer cap this file's own top comment derives. Named separately from
//     (and independent of) `image_decode.hpp::kMaxImageDecodeBytes` -- see that top comment for
//     why the two guards are deliberately different numbers guarding different things.
// PT: O teto de 64 MiB de buffer de pixel cru que o próprio comentário de topo deste arquivo
//     deriva. Nomeado separadamente de (e independente de) `image_decode.hpp::kMaxImageDecodeBytes`
//     -- ver aquele comentário de topo pro porquê dos dois guardas serem números deliberadamente
//     diferentes guardando coisas diferentes.
constexpr std::size_t kMaxImageEncodeBytes = 4096ull * 4096ull * 4ull; // 64 MiB.

// EN: stb_image_write's own `stbi_write_func` callback shape: `context` is whatever pointer the
//     caller handed to the `_to_func` call, `data`/`size` is one CHUNK of encoded output (never
//     the whole stream at once -- PNG in particular calls this once per zlib-compressed block).
//     Every format below hands this the SAME `EncodedImageBytes::bytes` vector's address as
//     `context`, so this one callback serves all five stb-backed formats.
// PT: A própria forma do callback `stbi_write_func` do stb_image_write: `context` é qualquer
//     ponteiro que o chamador entregou à chamada `_to_func`, `data`/`size` é UM PEDAÇO da saída
//     codificada (nunca o fluxo inteiro de uma vez -- o PNG em particular chama isto uma vez por
//     bloco comprimido em zlib). Todo formato abaixo entrega o endereço do MESMO vector
//     `EncodedImageBytes::bytes` como `context`, então este único callback serve os cinco
//     formatos apoiados em stb.
// EN: `data` is deliberately NOT `const void*` even though this callback never writes through it
//     (cppcheck's own `constParameterCallback` note below flags exactly that, and is right about
//     the local read-only usage) -- `&write_cb` is handed directly to stb_image_write's own
//     `_to_func` calls below, which require an EXACT match against its `stbi_write_func` typedef
//     (`void (*)(void*, void*, int)`, `stb_image_write.h`), a non-const `void*`; constifying the
//     parameter here would force a `reinterpret_cast<stbi_write_func*>` at every call site just
//     to silence a style note about a signature this file does not control.
// PT: `data` é deliberadamente NÃO `const void*` mesmo este callback nunca escrevendo através
//     dele (a própria nota `constParameterCallback` do cppcheck abaixo flagra exatamente isso, e
//     tem razão sobre o uso local só-leitura) -- `&write_cb` é entregue direto às próprias
//     chamadas `_to_func` do stb_image_write abaixo, que exigem correspondência EXATA contra o
//     próprio typedef `stbi_write_func` dele (`void (*)(void*, void*, int)`, `stb_image_write.h`),
//     um `void*` não-const; tornar o parâmetro const aqui forçaria um
//     `reinterpret_cast<stbi_write_func*>` em todo sítio de chamada só pra calar uma nota de
//     estilo sobre uma assinatura que este arquivo não controla. Suprimido no gate
//     (`--suppress='constParameterCallback:*/image_encode.cpp'`, `tools/precommit.sh` +
//     `.github/workflows/ci.yml`'s TST-L1-STATIC) em vez de um comentário inline
//     `cppcheck-suppress` -- este projeto não passa `--inline-suppr`, então um comentário
//     inline aqui seria silenciosamente ignorado (medido: o gate continuou falhando com ele
//     presente).
// PT (racional da supressão): suprimido no gate (`--suppress=
//     'constParameterCallback:*/image_encode.cpp'`, `tools/precommit.sh` +
//     `.github/workflows/ci.yml`'s TST-L1-STATIC) em vez de um comentário inline
//     `cppcheck-suppress` -- este projeto não passa `--inline-suppr`, então um comentário
//     inline aqui seria silenciosamente ignorado (medido: o gate continuou falhando com ele
//     presente).
void write_cb(void* context, void* data, int size) {
  if (context == nullptr || data == nullptr || size <= 0)
    return; // EN: defensive no-op; stb never calls this way in practice.
            // PT: no-op defensivo; o stb nunca chama assim na prática.
  auto* bytes = static_cast<std::vector<unsigned char>*>(context);
  const auto* chunk = static_cast<const unsigned char*>(data);
  bytes->insert(bytes->end(), chunk, chunk + size);
}

} // namespace

EncodedImageBytes encode_image_memory(ImageFormat format, int width, int height,
                                      const unsigned char* pixels,
                                      const EncodeImageOptions& options) {
  EncodedImageBytes out; // ok == false, bytes empty, by default.

  if (pixels == nullptr)
    return out;
  if (width <= 0 || height <= 0)
    return out;

  // EN: Overflow-safe `width * height * 4` computation, checked BEFORE `pixels` is ever
  //     dereferenced past index 0 -- same "guard before touching the buffer" discipline
  //     `image_decode.hpp`'s own `kMaxImageDecodeBytes` check uses (this file's own top comment
  //     explains why this is a DIFFERENT cap, on the raw-pixel side rather than the encoded-byte
  //     side). `width`/`height` are already known `> 0` here, so both divisors below are safe.
  // PT: Cálculo de `width * height * 4` seguro contra overflow, checado ANTES de `pixels` ser
  //     dereferenciado além do índice 0 -- mesma disciplina "guarda antes de tocar o buffer" que
  //     o próprio check de `kMaxImageDecodeBytes` de `image_decode.hpp` usa (o próprio comentário
  //     de topo deste arquivo explica por que este é um teto DIFERENTE, do lado do pixel cru em
  //     vez do byte codificado). `width`/`height` já são conhecidos `> 0` aqui, então os dois
  //     divisores abaixo são seguros.
  const std::size_t w = static_cast<std::size_t>(width);
  const std::size_t h = static_cast<std::size_t>(height);
  constexpr std::size_t kSizeMax = std::numeric_limits<std::size_t>::max();
  if (w > kSizeMax / h)
    return out; // EN: w * h would overflow. PT: w * h daria overflow.
  const std::size_t pixel_count = w * h;
  if (pixel_count > kSizeMax / 4u)
    return out; // EN: pixel_count * 4 would overflow. PT: pixel_count * 4 daria overflow.
  const std::size_t required_bytes = pixel_count * 4u;
  if (required_bytes > kMaxImageEncodeBytes)
    return out;

  switch (format) {
    case ImageFormat::Png: {
      const int ok = stbi_write_png_to_func(&write_cb, &out.bytes, width, height, 4, pixels,
                                            width * 4);
      out.ok = (ok != 0);
      break;
    }
    case ImageFormat::Jpg: {
      // EN: comp=4 handed straight to stb's own JPEG encoder -- it reads R/G/B from offsets
      //     0/1/2 of each `comp`-wide pixel and ignores the 4th component entirely (see
      //     `stb_image_write.h`'s own `stbi_write_jpg_core`, "comp == 2 is grey+alpha (alpha is
      //     ignored)" comment, which generalises identically to comp==4); no alpha-stripping
      //     copy is needed on this side.
      // PT: comp=4 entregue direto ao próprio encoder JPEG do stb -- ele lê R/G/B dos offsets
      //     0/1/2 de cada pixel de largura `comp` e ignora o 4º componente por completo (ver o
      //     próprio comentário "comp == 2 is grey+alpha (alpha is ignored)" do `stbi_write_jpg_core`
      //     de `stb_image_write.h`, que generaliza identicamente pra comp==4); nenhuma cópia de
      //     remoção de alpha é necessária deste lado.
      const int ok = stbi_write_jpg_to_func(&write_cb, &out.bytes, width, height, 4, pixels,
                                            options.jpg_quality);
      out.ok = (ok != 0);
      break;
    }
    case ImageFormat::Bmp: {
      const int ok = stbi_write_bmp_to_func(&write_cb, &out.bytes, width, height, 4, pixels);
      out.ok = (ok != 0);
      break;
    }
    case ImageFormat::Tga: {
      const int ok = stbi_write_tga_to_func(&write_cb, &out.bytes, width, height, 4, pixels);
      out.ok = (ok != 0);
      break;
    }
    case ImageFormat::Hdr: {
      // EN: See glintfx/image.hpp's own top comment (HDR paragraph) for the full "this is a
      //     declared downgrade, not real HDR capture" rationale. comp=3 (RGB only) -- the
      //     Radiance/RGBE format itself carries no alpha channel, so the 4th input component is
      //     read from `pixels` but never copied into `rgb` below.
      // PT: Ver o próprio comentário de topo de glintfx/image.hpp (parágrafo HDR) pro racional
      //     completo "isto é um downgrade declarado, não captura HDR real". comp=3 (só RGB) -- o
      //     próprio formato Radiance/RGBE não carrega canal alpha nenhum, então o 4º componente
      //     de input é lido de `pixels` mas nunca copiado pra `rgb` abaixo.
      std::vector<float> rgb(pixel_count * 3u);
      for (std::size_t i = 0; i < pixel_count; ++i) {
        rgb[i * 3 + 0] = static_cast<float>(pixels[i * 4 + 0]) / 255.0f;
        rgb[i * 3 + 1] = static_cast<float>(pixels[i * 4 + 1]) / 255.0f;
        rgb[i * 3 + 2] = static_cast<float>(pixels[i * 4 + 2]) / 255.0f;
      }
      const int ok = stbi_write_hdr_to_func(&write_cb, &out.bytes, width, height, 3, rgb.data());
      out.ok = (ok != 0);
      break;
    }
    case ImageFormat::Ppm: {
      // EN: P6 (binary RGB) -- the same textual header shape `App::snapshot()` writes
      //     (`app.cpp`), rewritten here rather than shared (that file's `snapshot()` also
      //     performs a GL-readback bottom-up row flip this module has no GL dependency to know
      //     about -- see glintfx/image.hpp's own top comment's "PIXEL CONVENTION" paragraph for
      //     why the two are not the same function in different clothes). Alpha is read (loop
      //     touches index `i*4+3` implicitly via the `+4` stride) but never written -- PPM has no
      //     alpha channel.
      // PT: P6 (RGB binário) -- a mesma forma de cabeçalho textual que o próprio
      //     `App::snapshot()` escreve (`app.cpp`), reescrita aqui em vez de compartilhada (o
      //     `snapshot()` daquele arquivo também faz um flip de linha bottom-up de readback GL que
      //     este módulo não tem dependência de GL nenhuma pra sequer saber que existe -- ver o
      //     parágrafo "CONVENÇÃO DE PIXEL" do próprio comentário de topo de glintfx/image.hpp pro
      //     porquê dos dois não serem a mesma função em roupas diferentes). O alpha é lido (o
      //     loop passa pelo stride `+4` implicitamente) mas nunca escrito -- PPM não tem canal
      //     alpha.
      char header[64];
      const int header_len = std::snprintf(header, sizeof(header), "P6\n%d %d\n255\n", width, height);
      out.bytes.reserve(out.bytes.size() + static_cast<std::size_t>(header_len > 0 ? header_len : 0) +
                        pixel_count * 3u);
      if (header_len > 0)
        out.bytes.insert(out.bytes.end(), header, header + header_len);
      for (std::size_t i = 0; i < pixel_count; ++i) {
        out.bytes.push_back(pixels[i * 4 + 0]);
        out.bytes.push_back(pixels[i * 4 + 1]);
        out.bytes.push_back(pixels[i * 4 + 2]);
      }
      out.ok = (header_len > 0);
      break;
    }
    case ImageFormat::Qoi: {
      qoi_encode_rgba(pixels, width, height, out.bytes);
      out.ok = true; // EN: qoi_encode_rgba() is total over this already-guarded input.
                     // PT: qoi_encode_rgba() é total sobre este input já guardado.
      break;
    }
    default:
      // EN: A hostile/malformed `static_cast<ImageFormat>(n)` outside the 7 declared
      //     enumerators -- rejected wholesale, never a fall-through into an arbitrary encoder.
      // PT: Um `static_cast<ImageFormat>(n)` hostil/malformado fora dos 7 enumeradores
      //     declarados -- rejeitado por completo, nunca um fall-through pra um encoder arbitrário.
      return EncodedImageBytes{};
  }

  if (!out.ok)
    out.bytes.clear(); // EN: never hand back a partial/corrupt buffer alongside ok == false.
                       // PT: nunca devolve um buffer parcial/corrompido junto de ok == false.
  return out;
}

bool encode_image_file(const char* path, ImageFormat format, int width, int height,
                       const unsigned char* pixels, const EncodeImageOptions& options) {
  if (path == nullptr || path[0] == '\0')
    return false;

  const EncodedImageBytes encoded = encode_image_memory(format, width, height, pixels, options);
  if (!encoded.ok)
    return false;

  // EN: D7-style ofstream idiom (binary, truncate any pre-existing file) -- opening a directory
  //     path or a path whose parent does not exist both fail to open on every platform this
  //     library targets, degrading to the SAME `false` this function already returns for every
  //     other guard above (no separate `stat()`/`is_directory()` pre-check needed).
  // PT: Idioma ofstream estilo D7 (binário, trunca qualquer arquivo pré-existente) -- abrir um
  //     path de diretório ou um path cujo pai não existe ambos falham ao abrir em toda
  //     plataforma que esta biblioteca mira, degradando pro MESMO `false` que esta função já
  //     devolve pra toda outra guarda acima (nenhum pré-check `stat()`/`is_directory()` separado
  //     é necessário).
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file)
    return false;

  file.write(reinterpret_cast<const char*>(encoded.bytes.data()),
             static_cast<std::streamsize>(encoded.bytes.size()));
  const bool write_ok = static_cast<bool>(file);
  file.close();

  if (!write_ok) {
    std::remove(path); // EN: best-effort; see this file's own top comment on why the return
                       //     value is ignored. PT: melhor-esforço; ver o próprio comentário de
                       //     topo deste arquivo pro porquê do valor de retorno ser ignorado.
    return false;
  }
  return true;
}

} // namespace glintfx
