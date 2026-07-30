// SPDX-License-Identifier: MPL-2.0
// EN: IMG-DECODE -- implementation of the public `glintfx/image.hpp` decode pair. Thin: both
//     functions delegate immediately to the private `image_decode.hpp` seam
//     (`decode_straight_rgba()`, IMG-DECODE's own straight-alpha sibling of that header's
//     pre-existing `decode_premultiplied_rgba()`) and convert its `DecodedImage` into the
//     public `DecodedImagePixels` -- a `std::move()` of the pixel vector, never a copy (DEC-MOVE,
//     W21: `decoded` below must NOT be `const` -- a `const` local defeats `std::move()` because
//     `std::move(decoded.rgba)` then yields `const vector&&`, which binds `operator=(const
//     vector&)`, a silent copy, not `operator=(vector&&)`; pinned by
//     `image_decode_hardening_sanity.cpp`'s own allocation-count oracle).
//
//     DEC-NOTHROW (W21): both functions below are `noexcept` -- `glintfx/image.hpp`'s own
//     FAIL-HIGH contract promises `ok == false` is the ONLY failure signal, "never a crash". The
//     one way that promise could be broken without a `try`/`catch` here: `std::bad_alloc` (or any
//     other `std::exception`) escaping from the vector allocation inside `decode_straight_rgba()`
//     (the `assign()` this file's own top comment already documents as irreducible -- see
//     `image_decode.hpp`'s own IMG-DECODE comment) or from this file's own `buf(len)` file-read
//     buffer. Each function below wraps its OWN allocation-bearing call in a `try`/`catch` that
//     degrades any `std::exception` (and, belt-and-suspenders, any other exception type at all --
//     the SAME "never a crash" discipline this file's callers rely on) to a clean, default-
//     constructed `DecodedImagePixels{}` (`ok == false`), never a partially-filled result. Proven
//     under a REAL, forced allocation failure (not a hypothetical) by
//     `image_decode_hardening_sanity.cpp`'s own `fork()` + `RLIMIT_AS` oracle.
//     `decode_image_file()`'s own file-read guard order is a LITERAL port of
//     `Draw2d::load_texture()`'s own ifstream idiom (`draw2d.cpp`): open, seek-to-end, check the
//     size BEFORE ever allocating a read buffer, seek back, read. See `glintfx/image.hpp`'s own
//     top comment for the full rationale (ownership, alpha convention, fail-high contract).
// PT: IMG-DECODE -- implementação do par público de decode de `glintfx/image.hpp`. Fina: as
//     duas funções delegam imediatamente pra costura privada `image_decode.hpp`
//     (`decode_straight_rgba()`, a própria irmã alpha-straight do IMG-DECODE do
//     `decode_premultiplied_rgba()` preexistente daquele header) e convertem o `DecodedImage`
//     dela no `DecodedImagePixels` público -- um `std::move()` do vector de pixel, nunca uma
//     cópia (DEC-MOVE, W21: `decoded` abaixo NÃO PODE ser `const` -- um local `const` derrota o
//     `std::move()` porque `std::move(decoded.rgba)` passa a render `const vector&&`, que liga em
//     `operator=(const vector&)`, uma cópia silenciosa, não em `operator=(vector&&)`; fixado pelo
//     próprio oráculo de contagem de alocação de `image_decode_hardening_sanity.cpp`).
//
//     DEC-FORMAT-SURFACE (W22, 2026-07-30): `decode_png_file()`, appended at the end of this
//     file, is a PNG-ONLY sibling of `decode_image_file()` -- see `glintfx/image.hpp`'s own
//     top-of-file addendum and that function's own doc-comment for the full rationale (the
//     82-byte fake-TGA-header coador the general 3-format dispatch tolerates). Reads the file
//     with the SAME literal-ported guard order as `decode_image_file()` (kept as an
//     independent copy on purpose, not a shared helper -- same "pure ADDITION, not a refactor
//     of working code" discipline this file already follows for its own two functions), then
//     rejects any buffer whose first 8 bytes are not the fixed PNG file signature BEFORE ever
//     delegating to `decode_image_memory()` above.
// PT: DEC-FORMAT-SURFACE (W22, 2026-07-30): `decode_png_file()`, somado no fim deste arquivo,
//     é uma irmã SÓ-PNG do `decode_image_file()` -- ver o próprio adendo de topo de
//     `glintfx/image.hpp` e o próprio doc-comment daquela função pro racional completo (o
//     coador de header-TGA-falso de 82 bytes que o dispatch geral de 3 formatos tolera). Lê o
//     arquivo com a MESMA ordem de guarda portada literalmente do `decode_image_file()`
//     (mantida como cópia independente de propósito, não um helper compartilhado -- mesma
//     disciplina "ADIÇÃO pura, não um refactor de código funcionando" que este arquivo já
//     segue pras próprias duas funções dele), depois rejeita qualquer buffer cujos primeiros
//     8 bytes não sejam a assinatura fixa de arquivo PNG ANTES de sequer delegar ao
//     `decode_image_memory()` acima.
//
//     DEC-NOTHROW (W21): as duas funções abaixo são `noexcept` -- o próprio contrato FAIL-HIGH de
//     `glintfx/image.hpp` promete que `ok == false` é o ÚNICO sinal de falha, "nunca um crash". A
//     UMA forma de aquela promessa quebrar sem um `try`/`catch` aqui: `std::bad_alloc` (ou
//     qualquer outro `std::exception`) escapando da alocação de vector dentro de
//     `decode_straight_rgba()` (o `assign()` que o próprio comentário de topo deste arquivo já
//     documenta como irredutível -- ver o próprio comentário IMG-DECODE de `image_decode.hpp`) ou
//     do próprio buffer de leitura de arquivo `buf(len)` deste arquivo. Cada função abaixo
//     envolve a PRÓPRIA chamada portadora de alocação num `try`/`catch` que degrada qualquer
//     `std::exception` (e, cinto-e-suspensório, qualquer outro tipo de exceção também -- a MESMA
//     disciplina "nunca um crash" da qual os chamadores deste arquivo dependem) pra um
//     `DecodedImagePixels{}` limpo, default-construído (`ok == false`), nunca um resultado
//     parcialmente preenchido. Provado sob uma falha de alocação REAL, forçada (não hipotética)
//     pelo próprio oráculo `fork()` + `RLIMIT_AS` de `image_decode_hardening_sanity.cpp`.
//     A própria ordem de guarda de leitura de arquivo do `decode_image_file()` é um port
//     literal do idioma ifstream do próprio `Draw2d::load_texture()` (`draw2d.cpp`): abre,
//     seek-até-o-fim, checa o tamanho ANTES de sequer alocar um buffer de leitura, volta o seek,
//     lê. Ver o próprio comentário do topo de `glintfx/image.hpp` pro racional completo (posse,
//     convenção de alpha, contrato fail-high).
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include "image_decode.hpp"

#include <exception>
#include <fstream>
#include <ios>
#include <new>

namespace glintfx {

DecodedImagePixels decode_image_memory(const unsigned char* data, std::size_t len) noexcept {
  DecodedImagePixels out; // ok == false by default.
  try {
    // EN: NOT `const` -- see this file's own top comment (DEC-MOVE) for why a `const` local here
    //     silently defeats the `std::move()` two lines below.
    // PT: NÃO `const` -- ver o próprio comentário de topo deste arquivo (DEC-MOVE) pro porquê de
    //     um local `const` aqui derrotar silenciosamente o `std::move()` duas linhas abaixo.
    DecodedImage decoded = decode_straight_rgba(data, len);
    if (!decoded.ok)
      return out;
    out.pixels = std::move(decoded.rgba); // move: no allocation, steals decoded.rgba's buffer.
    out.width = decoded.width;
    out.height = decoded.height;
    out.ok = true;
  } catch (const std::exception&) {
    // EN: std::bad_alloc (the expected case, from decode_straight_rgba()'s own vector assign())
    //     or any other std::exception -- degrade to a clean, default-constructed result, never a
    //     partially-filled `out`. See this file's own top comment (DEC-NOTHROW).
    // PT: std::bad_alloc (o caso esperado, do próprio assign() de vector de
    //     decode_straight_rgba()) ou qualquer outro std::exception -- degrada pra um resultado
    //     limpo, default-construído, nunca um `out` parcialmente preenchido. Ver o próprio
    //     comentário de topo deste arquivo (DEC-NOTHROW).
    return DecodedImagePixels{};
  } catch (...) {
    // EN: Belt-and-suspenders: an exception type that is not a std::exception at all (e.g. from a
    //     future third-party dependency this seam might grow) must not escape this noexcept
    //     boundary either -- same "never a crash" discipline, no exception.
    // PT: Cinto-e-suspensório: um tipo de exceção que nem é um std::exception (ex.: de uma
    //     dependência de terceiro futura que esta costura possa ganhar) também não pode escapar
    //     desta fronteira noexcept -- mesma disciplina "nunca um crash", sem exceção.
    return DecodedImagePixels{};
  }
  return out;
}

DecodedImagePixels decode_image_file(const char* path) noexcept {
  DecodedImagePixels out; // ok == false by default.
  if (path == nullptr)
    return out;

  // D7-style ifstream idiom, LITERAL port of Draw2d::load_texture()'s own (draw2d.cpp) --
  // const char* overload avoids std::filesystem::path::c_str()'s wchar_t trap on MSVC.
  // ifstream's default exception mask (std::ios::goodbit) means none of the stream operations
  // below ever throw -- only the `buf(len)` allocation and the decode_image_memory() delegation
  // (already noexcept-safe on its own, see above) can, hence the try/catch scoped to just those.
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return out;
  file.seekg(0, std::ios::end);
  const std::streamoff len_off = file.tellg();
  if (len_off < 0)
    return out;
  const std::size_t len = static_cast<std::size_t>(len_off);
  // 256 MiB cap (kMaxImageDecodeBytes, image_decode.hpp) checked BEFORE ever allocating a read
  // buffer for a hostile/huge file -- same "guard 1/2, pre-allocation" idiom load_texture() uses.
  if (len == 0 || len > kMaxImageDecodeBytes)
    return out;
  file.seekg(0, std::ios::beg);

  try {
    std::vector<unsigned char> buf(len);
    if (!file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(len)))
      return out;
    return decode_image_memory(buf.data(), buf.size());
  } catch (const std::exception&) {
    // EN/PT: same DEC-NOTHROW discipline as decode_image_memory() above -- the `buf(len)`
    // allocation is this function's OWN failure mode (decode_image_memory()'s own failures are
    // already caught inside that function and never reach this catch at all).
    return DecodedImagePixels{};
  } catch (...) {
    return DecodedImagePixels{};
  }
}

namespace {

// EN: DEC-FORMAT-SURFACE (W22) -- the fixed 8-byte PNG file signature (PNG specification,
//     RFC 2083 / ISO 15948 -- the SAME 8 bytes every PNG encoder in existence writes as the
//     first 8 bytes of the file). See `decode_png_file()` below and `glintfx/image.hpp`'s own
//     top-of-file DEC-FORMAT-SURFACE addendum for why this check exists and what it does (and
//     does not) guard.
// PT: DEC-FORMAT-SURFACE (W22) -- a assinatura fixa de 8 bytes de arquivo PNG (especificação
//     PNG, RFC 2083 / ISO 15948 -- os MESMOS 8 bytes que todo encoder de PNG já existente
//     escreve como os primeiros 8 bytes do arquivo). Ver o `decode_png_file()` abaixo e o
//     próprio adendo de topo DEC-FORMAT-SURFACE de `glintfx/image.hpp` pro porquê desta
//     checagem existir e o que ela guarda (e não guarda).
constexpr unsigned char kPngSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

// EN: Total, pure, cannot log (same discipline as every other helper in this module) --
//     `data == nullptr` or `len` shorter than the 8-byte signature is a clean `false`, never a
//     read past the end of a short/hostile buffer.
// PT: Total, pura, não pode logar (mesma disciplina de todo outro helper deste módulo) --
//     `data == nullptr` ou `len` mais curto que a assinatura de 8 bytes é um `false` limpo,
//     nunca uma leitura além do fim de um buffer curto/hostil.
bool has_png_signature(const unsigned char* data, std::size_t len) noexcept {
  if (data == nullptr || len < sizeof(kPngSignature))
    return false;
  for (std::size_t i = 0; i < sizeof(kPngSignature); ++i) {
    if (data[i] != kPngSignature[i])
      return false;
  }
  return true;
}

} // namespace

DecodedImagePixels decode_png_file(const char* path) noexcept {
  DecodedImagePixels out; // ok == false by default.
  if (path == nullptr)
    return out;

  // EN: Literal port of decode_image_file()'s own file-read guard order above (open,
  //     seek-to-end, size check BEFORE ever allocating a read buffer, seek back, read) -- see
  //     this file's own top comment (DEC-FORMAT-SURFACE) for why this stays a SEPARATE copy
  //     rather than a shared helper.
  // PT: Port literal da própria ordem de guarda de leitura de arquivo do decode_image_file()
  //     acima (abre, seek-até-o-fim, checa tamanho ANTES de sequer alocar um buffer de
  //     leitura, volta o seek, lê) -- ver o próprio comentário de topo deste arquivo
  //     (DEC-FORMAT-SURFACE) pro porquê disto ficar como cópia SEPARADA em vez de helper
  //     compartilhado.
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return out;
  file.seekg(0, std::ios::end);
  const std::streamoff len_off = file.tellg();
  if (len_off < 0)
    return out;
  const std::size_t len = static_cast<std::size_t>(len_off);
  if (len == 0 || len > kMaxImageDecodeBytes)
    return out;
  file.seekg(0, std::ios::beg);

  try {
    std::vector<unsigned char> buf(len);
    if (!file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(len)))
      return out;
    // EN: DEC-FORMAT-SURFACE's own gate -- reject BEFORE the buffer ever reaches the shared
    //     PNG/JPG/TGA decode seam (decode_image_memory() below, which would otherwise happily
    //     decode a non-PNG buffer -- including the exact 82-byte fake-TGA-header case named in
    //     glintfx/image.hpp's own top-of-file addendum).
    // PT: A PRÓPRIA guarda do DEC-FORMAT-SURFACE -- rejeita ANTES do buffer sequer alcançar a
    //     costura de decode PNG/JPG/TGA compartilhada (decode_image_memory() abaixo, que senão
    //     decodificaria de bom grado um buffer não-PNG -- inclusive o caso exato de header-TGA-
    //     falso de 82 bytes nomeado no próprio adendo de topo de glintfx/image.hpp).
    if (!has_png_signature(buf.data(), buf.size()))
      return out;
    return decode_image_memory(buf.data(), buf.size());
  } catch (const std::exception&) {
    // EN/PT: same DEC-NOTHROW discipline as decode_image_file() above -- the buf(len)
    // allocation is this function's OWN failure mode.
    return DecodedImagePixels{};
  } catch (...) {
    return DecodedImagePixels{};
  }
}

} // namespace glintfx
