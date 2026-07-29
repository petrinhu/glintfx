// SPDX-License-Identifier: MPL-2.0
// EN: IMG-DECODE -- implementation of the public `glintfx/image.hpp` decode pair. Thin: both
//     functions delegate immediately to the private `image_decode.hpp` seam
//     (`decode_straight_rgba()`, IMG-DECODE's own straight-alpha sibling of that header's
//     pre-existing `decode_premultiplied_rgba()`) and convert its `DecodedImage` into the
//     public `DecodedImagePixels` -- a `std::move()` of the pixel vector, never a copy.
//     `decode_image_file()`'s own file-read guard order is a LITERAL port of
//     `Draw2d::load_texture()`'s own ifstream idiom (`draw2d.cpp`): open, seek-to-end, check the
//     size BEFORE ever allocating a read buffer, seek back, read. See `glintfx/image.hpp`'s own
//     top comment for the full rationale (ownership, alpha convention, fail-high contract).
// PT: IMG-DECODE -- implementação do par público de decode de `glintfx/image.hpp`. Fina: as
//     duas funções delegam imediatamente pra costura privada `image_decode.hpp`
//     (`decode_straight_rgba()`, a própria irmã alpha-straight do IMG-DECODE do
//     `decode_premultiplied_rgba()` preexistente daquele header) e convertem o `DecodedImage`
//     dela no `DecodedImagePixels` público -- um `std::move()` do vector de pixel, nunca uma
//     cópia. A própria ordem de guarda de leitura de arquivo do `decode_image_file()` é um port
//     literal do idioma ifstream do próprio `Draw2d::load_texture()` (`draw2d.cpp`): abre,
//     seek-até-o-fim, checa o tamanho ANTES de sequer alocar um buffer de leitura, volta o seek,
//     lê. Ver o próprio comentário do topo de `glintfx/image.hpp` pro racional completo (posse,
//     convenção de alpha, contrato fail-high).
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include "image_decode.hpp"

#include <fstream>
#include <ios>

namespace glintfx {

DecodedImagePixels decode_image_memory(const unsigned char* data, std::size_t len) {
  DecodedImagePixels out; // ok == false by default.
  const DecodedImage decoded = decode_straight_rgba(data, len);
  if (!decoded.ok)
    return out;
  out.ok = true;
  out.width = decoded.width;
  out.height = decoded.height;
  out.pixels = std::move(decoded.rgba);
  return out;
}

DecodedImagePixels decode_image_file(const char* path) {
  DecodedImagePixels out; // ok == false by default.
  if (path == nullptr)
    return out;

  // D7-style ifstream idiom, LITERAL port of Draw2d::load_texture()'s own (draw2d.cpp) --
  // const char* overload avoids std::filesystem::path::c_str()'s wchar_t trap on MSVC.
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
  std::vector<unsigned char> buf(len);
  if (!file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(len)))
    return out;

  return decode_image_memory(buf.data(), buf.size());
}

} // namespace glintfx
