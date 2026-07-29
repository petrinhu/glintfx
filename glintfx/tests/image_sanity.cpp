// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx/image.hpp (IMG-DECODE, the public decode_image_file()/
//     decode_image_memory() pair). No RmlUi, no GL, no window, no `Draw2d`/`App`/`UiLayer`
//     instance -- both functions under test are free functions callable headless, same "no
//     display to isolate here" reasoning as image_decode_sanity.cpp/input_state_sanity.cpp.
//     Registered unconditionally -- runs in BOTH build configurations.
//
//     THE ONE THING THIS FILE EXISTS TO PROVE THAT image_decode_sanity.cpp DOES NOT: the public
//     API returns STRAIGHT (non-premultiplied) alpha, a DELIBERATE divergence from the private
//     `image_decode.hpp::decode_premultiplied_rgba()` that file already tests -- see
//     `glintfx/image.hpp`'s own top comment for the full rationale. Reuses the SAME
//     image_decode_rgba_2x2.png fixture image_decode_sanity.cpp already established (same build
//     dir, same COPYONLY convention, no second copy configured) precisely so the two files'
//     results can be compared BY VALUE at the same pixels: where `decode_premultiplied_rgba()`
//     zeroes the fully-transparent pixel's RGB and halves the half-alpha pixel's RGB,
//     `decode_image_memory()`/`decode_image_file()` below must leave BOTH untouched.
//
//     Hostile corpus mirrors image_decode_sanity.cpp's own (png_truncated/garbage/huge_dims/
//     over-cap len/null+zero-len, re-derived here rather than shared -- same "different test
//     binary, no shared header for these" convention that file's own top comment states) PLUS
//     the two failure modes unique to `decode_image_file()`'s own file-I/O step: a null path and
//     a path to a file that does not exist.
// PT: Teste unit puro para glintfx/image.hpp (IMG-DECODE, o par público de decode
//     decode_image_file()/decode_image_memory()). Sem RmlUi, sem GL, sem janela, sem instância
//     `Draw2d`/`App`/`UiLayer` -- as duas funções sob teste são free functions chamáveis
//     headless, mesma racional "nada relacionado a display pra isolar aqui" de
//     image_decode_sanity.cpp/input_state_sanity.cpp. Registrado incondicionalmente -- roda nas
//     DUAS configurações de build.
//
//     A ÚNICA COISA QUE ESTE ARQUIVO EXISTE PRA PROVAR QUE image_decode_sanity.cpp NÃO PROVA: a
//     API pública devolve alpha STRAIGHT (não-premultiplicado), uma divergência DELIBERADA do
//     `image_decode.hpp::decode_premultiplied_rgba()` privado que aquele arquivo já testa -- ver
//     o próprio comentário do topo de `glintfx/image.hpp` pro racional completo. Reusa a MESMA
//     fixture image_decode_rgba_2x2.png que image_decode_sanity.cpp já estabeleceu (mesmo build
//     dir, mesma convenção COPYONLY, nenhuma segunda cópia configurada) exatamente pra que os
//     resultados dos dois arquivos possam ser comparados POR VALOR nos mesmos pixels: onde
//     `decode_premultiplied_rgba()` zera o RGB do pixel totalmente-transparente e reduz à metade
//     o RGB do pixel meio-alpha, `decode_image_memory()`/`decode_image_file()` abaixo precisam
//     deixar OS DOIS intocados.
//
//     Corpus hostil espelha o próprio de image_decode_sanity.cpp (png_truncated/garbage/
//     huge_dims/len-acima-do-teto/null+zero-len, re-derivado aqui em vez de compartilhado --
//     mesma convenção "binário de teste diferente, sem header compartilhado pra isto" que o
//     próprio comentário de topo daquele arquivo declara) MAIS os dois modos de falha exclusivos
//     do próprio passo de I/O de arquivo do `decode_image_file()`: um path nulo e um path pra um
//     arquivo que não existe.
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// ---------------------------------------------------------------------------
// EN: Hostile-corpus builders, re-derived from image_decode_sanity.cpp's own (see this file's
//     top comment for why re-deriving, not sharing, is deliberate here).
// PT: Construtores de corpus hostil, re-derivados dos próprios de image_decode_sanity.cpp (ver
//     o comentário de topo deste arquivo pro porquê de re-derivar, não compartilhar, ser
//     deliberado aqui).
// ---------------------------------------------------------------------------

void put_be32(std::vector<unsigned char>& v, uint32_t x) {
  v.push_back(static_cast<unsigned char>((x >> 24) & 0xFF));
  v.push_back(static_cast<unsigned char>((x >> 16) & 0xFF));
  v.push_back(static_cast<unsigned char>((x >> 8) & 0xFF));
  v.push_back(static_cast<unsigned char>(x & 0xFF));
}

void put_tag(std::vector<unsigned char>& v, const char (&tag)[5]) {
  v.insert(v.end(), tag, tag + 4);
}

const unsigned char kPngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

std::vector<unsigned char> build_ihdr_chunk(uint32_t w, uint32_t h, uint8_t depth, uint8_t color) {
  std::vector<unsigned char> data;
  put_be32(data, w);
  put_be32(data, h);
  data.push_back(depth);
  data.push_back(color);
  data.push_back(0);
  data.push_back(0);
  data.push_back(0);

  std::vector<unsigned char> chunk;
  put_be32(chunk, static_cast<uint32_t>(data.size()));
  put_tag(chunk, "IHDR");
  chunk.insert(chunk.end(), data.begin(), data.end());
  put_be32(chunk, 0);
  return chunk;
}

std::vector<unsigned char> build_png_truncated() {
  std::vector<unsigned char> f(kPngSig, kPngSig + 8);
  const auto ihdr = build_ihdr_chunk(4, 4, 8, 6);
  f.insert(f.end(), ihdr.begin(), ihdr.end());
  put_be32(f, 100);
  put_tag(f, "IDAT");
  for (int i = 0; i < 10; ++i)
    f.push_back(static_cast<unsigned char>(0xAA ^ i));
  return f;
}

std::vector<unsigned char> build_garbage() {
  std::vector<unsigned char> f;
  f.reserve(4096);
  for (int i = 0; i < 4096; ++i)
    f.push_back(static_cast<unsigned char>((i * 167 + 31) & 0xFF));
  return f;
}

std::vector<unsigned char> build_huge_dims() {
  std::vector<unsigned char> f(kPngSig, kPngSig + 8);
  const auto ihdr = build_ihdr_chunk(100000, 100000, 8, 6);
  f.insert(f.end(), ihdr.begin(), ihdr.end());
  return f;
}

} // namespace

using namespace glintfx;

int main() {
  // ===========================================================================================
  // EN: Group 1 -- decode_image_memory() on the SAME image_decode_rgba_2x2.png fixture bytes
  //     image_decode_sanity.cpp decodes with decode_premultiplied_rgba(). Straight-alpha contract
  //     proven BY VALUE at the two pixels premultiply would otherwise change:
  //       (x=1,y=0) RGBA(200,100,50,  0) -> decode_premultiplied_rgba zeroes RGB to (0,0,0,0);
  //                 decode_image_memory MUST leave it (200,100,50,0), byte-exact.
  //       (x=0,y=1) RGBA( 10, 20, 30,128) -> decode_premultiplied_rgba halves RGB to (5,10,15,128);
  //                 decode_image_memory MUST leave it (10,20,30,128), byte-exact.
  //     The other two pixels (opaque, opaque white) are premultiply no-ops either way, so they do
  //     not discriminate the two functions -- checked anyway for completeness.
  // PT: Grupo 1 -- decode_image_memory() sobre os MESMOS bytes da fixture
  //     image_decode_rgba_2x2.png que image_decode_sanity.cpp decodifica com
  //     decode_premultiplied_rgba(). Contrato alpha-straight provado POR VALOR nos dois pixels
  //     que o premultiply mudaria de outra forma (ver EN acima pros valores). Os outros dois
  //     pixels (opaco, branco opaco) são no-op de premultiply nos dois casos, então não
  //     discriminam as duas funções -- checados mesmo assim por completude.
  // ===========================================================================================
  {
    const DecodedImagePixels decoded = decode_image_file("image_decode_rgba_2x2.png");
    check(decoded.ok, "png fixture (file): decode_image_file reports ok == true");
    check(decoded.width == 2, "png fixture (file): width == 2");
    check(decoded.height == 2, "png fixture (file): height == 2");
    check(decoded.pixels.size() == 16, "png fixture (file): pixels buffer is width*height*4 == 16 bytes");

    if (decoded.ok && decoded.width == 2 && decoded.height == 2 && decoded.pixels.size() == 16) {
      const unsigned char* px = decoded.pixels.data();
      const unsigned char expect_opaque[4] = {200, 100, 50, 255};
      const unsigned char expect_transparent_straight[4] = {200, 100, 50, 0}; // NOT zeroed.
      const unsigned char expect_half_straight[4] = {10, 20, 30, 128};        // NOT halved.
      const unsigned char expect_white[4] = {255, 255, 255, 255};

      auto pixel_matches = [&](int i, const unsigned char expect[4]) {
        return px[i * 4 + 0] == expect[0] && px[i * 4 + 1] == expect[1] &&
               px[i * 4 + 2] == expect[2] && px[i * 4 + 3] == expect[3];
      };
      check(pixel_matches(0, expect_opaque), "png fixture (file): (0,0) opaque pixel unchanged");
      check(pixel_matches(1, expect_transparent_straight),
            "png fixture (file): (1,0) transparent pixel RGB left STRAIGHT, "
            "NOT zeroed (proves this is not the premultiplied decode)");
      check(pixel_matches(2, expect_half_straight),
            "png fixture (file): (0,1) half-alpha pixel RGB left STRAIGHT, "
            "NOT halved (proves this is not the premultiplied decode)");
      check(pixel_matches(3, expect_white), "png fixture (file): (1,1) opaque white pixel unchanged");
    }
  }

  // ===========================================================================================
  // EN: Group 2 -- decode_image_memory() called directly on the JPEG fixture's own bytes (the
  //     memory-buffer entry point decode_image_file() itself delegates to), confirming both
  //     public entry points reach the same decode path. Lossy format: tolerance on RGB, exact on
  //     alpha (same convention image_decode_sanity.cpp's own JPEG group uses).
  // PT: Grupo 2 -- decode_image_memory() chamada direto sobre os próprios bytes da fixture JPEG
  //     (o ponto de entrada de buffer-em-memória pro qual o próprio decode_image_file() delega),
  //     confirmando que os dois pontos de entrada públicos alcançam o mesmo caminho de decode.
  //     Formato com perda: tolerância em RGB, exato em alpha (mesma convenção que o próprio
  //     grupo JPEG de image_decode_sanity.cpp usa).
  // ===========================================================================================
  {
    std::FILE* f = std::fopen("image_decode_solid.jpg", "rb");
    check(f != nullptr, "jpg fixture (memory): image_decode_solid.jpg opened (missing from build dir?)");
    std::vector<unsigned char> bytes;
    if (f) {
      std::fseek(f, 0, SEEK_END);
      const long len = std::ftell(f);
      std::fseek(f, 0, SEEK_SET);
      if (len > 0) {
        bytes.resize(static_cast<size_t>(len));
        const size_t n = std::fread(bytes.data(), 1, bytes.size(), f);
        if (n != bytes.size())
          bytes.clear();
      }
      std::fclose(f);
    }
    check(!bytes.empty(), "jpg fixture (memory): bytes read");

    const DecodedImagePixels decoded = decode_image_memory(bytes.data(), bytes.size());
    check(decoded.ok, "jpg fixture (memory): decode_image_memory reports ok == true");
    check(decoded.width == 4, "jpg fixture (memory): width == 4");
    check(decoded.height == 4, "jpg fixture (memory): height == 4");
    check(decoded.pixels.size() == 64, "jpg fixture (memory): pixels buffer is width*height*4 == 64 bytes");

    if (decoded.ok && decoded.pixels.size() == 64) {
      const int expect_r = 100, expect_g = 150, expect_b = 200;
      const int tolerance = 12;
      bool rgb_ok = true, alpha_ok = true;
      for (size_t i = 0; i < 16; ++i) {
        const unsigned char* p = &decoded.pixels[i * 4];
        const int dr = static_cast<int>(p[0]) - expect_r;
        const int dg = static_cast<int>(p[1]) - expect_g;
        const int db = static_cast<int>(p[2]) - expect_b;
        if ((dr < -tolerance || dr > tolerance) || (dg < -tolerance || dg > tolerance) ||
            (db < -tolerance || db > tolerance))
          rgb_ok = false;
        if (p[3] != 255)
          alpha_ok = false;
      }
      check(rgb_ok, "jpg fixture (memory): every pixel's RGB within tolerance of the source solid colour");
      check(alpha_ok, "jpg fixture (memory): every pixel's A == 255 (no native alpha in JPEG)");
    }
  }

  // ===========================================================================================
  // EN: Group 3 -- hostile corpus, decode_image_memory() side. Every case must return
  //     ok == false; none may crash/UB (run under ASan in the review/CI leg).
  // PT: Grupo 3 -- corpus hostil, lado decode_image_memory(). Todo caso precisa retornar
  //     ok == false; nenhum pode crashar/UB (roda sob ASan na perna de review/CI).
  // ===========================================================================================
  {
    const auto bytes = build_png_truncated();
    check(!decode_image_memory(bytes.data(), bytes.size()).ok,
          "hostile (memory): png_truncated rejected (ok == false)");
  }
  {
    const auto bytes = build_garbage();
    check(!decode_image_memory(bytes.data(), bytes.size()).ok,
          "hostile (memory): garbage rejected (ok == false)");
  }
  {
    const auto bytes = build_huge_dims();
    check(!decode_image_memory(bytes.data(), bytes.size()).ok,
          "hostile (memory): huge_dims rejected (ok == false)");
  }
  {
    const std::vector<unsigned char> empty;
    check(!decode_image_memory(empty.data(), empty.size()).ok,
          "hostile (memory): zero-byte input rejected (ok == false)");
  }
  {
    // EN: The public API's own 256 MiB cap is the SAME kMaxImageDecodeBytes the private seam
    //     enforces (image_decode.hpp), re-exercised here at the public boundary. `data` is a REAL
    //     (tiny) buffer, `len` claims one byte past the cap -- safe because the cap check runs
    //     BEFORE `data` is ever dereferenced.
    // PT: O teto próprio de 256 MiB da API pública é o MESMO kMaxImageDecodeBytes que a costura
    //     privada aplica (image_decode.hpp), re-exercitado aqui na fronteira pública. `data` é um
    //     buffer REAL (minúsculo), `len` reivindica um byte além do teto -- seguro porque a
    //     checagem do teto roda ANTES de `data` sequer ser dereferenciado.
    const unsigned char tiny[4] = {0, 0, 0, 0};
    check(!decode_image_memory(tiny, 256u * 1024u * 1024u + 1u).ok,
          "hostile (memory): len one byte past the 256 MiB cap rejected (ok == false)");
  }
  {
    check(!decode_image_memory(nullptr, 0).ok, "hostile (memory): null data + zero len rejected (ok == false)");
    const unsigned char tiny[4] = {0, 0, 0, 0};
    check(!decode_image_memory(tiny, 0).ok, "hostile (memory): zero len with non-null data rejected (ok == false)");
    check(!decode_image_memory(nullptr, 4).ok, "hostile (memory): null data with non-zero len rejected (ok == false)");
  }

  // ===========================================================================================
  // EN: Group 4 -- hostile corpus, decode_image_file() side: the two failure modes unique to the
  //     file-I/O entry point (a null path, a path that does not resolve to an existing file).
  //     Also: a real decode_image_file() call still works right after the corpus (same "still
  //     usable" oracle idiom draw2d_hostile_sanity.cpp's own hostile corpora use).
  // PT: Grupo 4 -- corpus hostil, lado decode_image_file(): os dois modos de falha exclusivos do
  //     ponto de entrada de I/O de arquivo (um path nulo, um path que não resolve pra um arquivo
  //     existente). Também: uma chamada real decode_image_file() ainda funciona logo após o
  //     corpus (mesmo idioma-oráculo "ainda usável" que os próprios corpos hostis de
  //     draw2d_hostile_sanity.cpp usam).
  // ===========================================================================================
  {
    check(!decode_image_file(nullptr).ok, "hostile (file): nullptr path rejected (ok == false)");
    check(!decode_image_file("this/path/does/not/exist.png").ok,
          "hostile (file): nonexistent path rejected (ok == false)");
    check(!decode_image_file("").ok, "hostile (file): empty-string path rejected (ok == false)");

    const DecodedImagePixels real = decode_image_file("image_decode_rgba_2x2.png");
    check(real.ok && real.width == 2 && real.height == 2,
          "hostile (file): a real decode_image_file() call still works right after the corpus");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "image_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("image_sanity: PASS");
  return 0;
}
