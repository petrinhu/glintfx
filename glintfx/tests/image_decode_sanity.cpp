// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for src/image_decode.hpp (D2D-1A, the image-decode carve --
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md, decision D3). No RmlUi, no GL, no
//     window -- decode_premultiplied_rgba() is a plain "bytes in, pixels out" function, called
//     DIRECTLY here (unlike texture_png_alpha.cpp / asset_decode_hostile_sanity.cpp, which only
//     exercise decode indirectly through an App/UiLayer document load). Registered unconditionally
//     (same "no display to isolate here" reasoning as input_state_sanity.cpp/log_dedup_sanity.cpp)
//     -- runs in BOTH build configurations.
//
//     Two groups of cases:
//       1. Valid decode + premultiply verified BY VALUE (not just "did not crash"):
//          - image_decode_rgba_2x2.png: a hand-authored 2x2 RGBA PNG (generated once via PIL, see
//            this file's own commit) with one pixel per premultiply case the formula in
//            image_decode.hpp hits: fully opaque (no-op), fully transparent with non-zero RGB
//            (the halo bug this whole mechanism exists to prevent -- RGB must become exactly
//            zero), half alpha (a=128, exercises the integer-division rounding path, expected
//            value derived by hand in this file's own comment below), and opaque white (boundary
//            channel values). PNG is lossless, so every one of these four pixels is checked for
//            an EXACT byte match, not a tolerance.
//          - image_decode_solid.jpg: a 4x4 solid-colour JPEG (opaque, no native alpha channel --
//            stb_image forces A=255 on any JPEG regardless of source). JPEG is lossy even at
//            quality 100 (DCT/chroma-subsampling artifacts), so RGB is checked with a small
//            tolerance (never pixel-exact, same "declared downgrade" discipline as render_sanity/
//            golden_test's own llvmpipe non-determinism note) -- but A==255 for every pixel is
//            checked EXACTLY (stb's own hard invariant for alpha-less source formats), and because
//            a=255 makes the premultiply formula an exact per-channel identity
//            (X*255/255 == X for any integer X, no rounding loss when the alpha channel is
//            already opaque), the RGB tolerance check also incidentally proves premultiply did
//            NOT corrupt an opaque pixel.
//       2. Hostile corpus reused AT THE HELPER SEAM (D2D-1A's own mandate): the same malformed-
//          PNG shapes asset_decode_hostile_sanity.cpp exercises indirectly through a document
//          load (png_truncated, garbage, huge_dims) are rebuilt here as local byte buffers and
//          handed to decode_premultiplied_rgba() DIRECTLY -- proving the helper itself rejects
//          them (ok == false), not merely that some enclosing document-load path degrades
//          gracefully. Plus two cases specific to the helper's OWN 256 MiB cap
//          (kMaxImageDecodeBytes, independent of render_gl3.cpp's `kMaxAssetFileBytes` --
//          see image_decode.hpp's own top comment for why the two guards are deliberately
//          separate): an over-cap `len` against a REAL (but far smaller) backing buffer -- safe
//          because the cap check runs before `data` is ever dereferenced, so the mismatched
//          buffer size is never touched -- and a null-data/zero-len pair.
//
// PT: Teste unit puro para src/image_decode.hpp (D2D-1A, o carve do decode de imagem --
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md, decisão D3). Sem RmlUi, sem GL, sem
//     janela -- decode_premultiplied_rgba() é uma função simples "bytes entram, pixels saem",
//     chamada DIRETAMENTE aqui (diferente de texture_png_alpha.cpp / asset_decode_hostile_
//     sanity.cpp, que só exercitam o decode indiretamente via load de documento num App/UiLayer).
//     Registrado incondicionalmente (mesma racional "nada relacionado a display pra isolar aqui"
//     de input_state_sanity.cpp/log_dedup_sanity.cpp) -- roda nas DUAS configurações de build.
//
//     Dois grupos de casos:
//       1. Decode válido + premultiply verificado POR VALOR (não só "não crashou"): ver os
//          parágrafos EN acima (espelhados aqui em conceito) -- PNG 2x2 com um pixel por caso de
//          premultiply (opaco, transparente com RGB não-zero, meio-alpha, branco opaco), checado
//          BYTE-EXATO (PNG é sem perda); JPEG 4x4 sólido, opaco, checado com tolerância pequena de
//          RGB (JPEG é com perda mesmo em qualidade 100) mas A==255 EXATO.
//       2. Corpus hostil reusado NA COSTURA DO HELPER: as mesmas formas malformadas de PNG que
//          asset_decode_hostile_sanity.cpp exercita indiretamente (png_truncated, garbage,
//          huge_dims) reconstruídas aqui como buffers locais e entregues DIRETO ao
//          decode_premultiplied_rgba() -- prova que o PRÓPRIO helper rejeita (ok == false), não
//          só que algum caminho de load de documento ao redor degrada graciosamente. Mais dois
//          casos específicos do teto PRÓPRIO de 256 MiB do helper (kMaxImageDecodeBytes,
//          independente do `kMaxAssetFileBytes` de render_gl3.cpp -- ver o próprio comentário de
//          topo de image_decode.hpp pro porquê das duas guardas serem deliberadamente separadas).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/image_decode.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Reads a file (copied to WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR by tests/CMakeLists.txt
//     at configure time, same convention as texture_png_alpha.cpp) into a byte buffer. Empty
//     vector on any I/O failure -- the caller's own `check()` on the resulting decode catches a
//     missing fixture (a fixture missing from the build dir is itself a build-system bug worth
//     failing loudly on, not silently skipping).
// PT: Lê um arquivo (copiado para WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR pelo
//     tests/CMakeLists.txt em tempo de configure, mesma convenção de texture_png_alpha.cpp) num
//     buffer de bytes. Vetor vazio em qualquer falha de I/O -- o próprio `check()` do chamador
//     sobre o decode resultante captura uma fixture ausente (uma fixture ausente do build dir já
//     é em si um bug de build-system que merece falhar alto, não pular em silêncio).
std::vector<unsigned char> read_file(const char* path) {
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return {};
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

// ---------------------------------------------------------------------------
// EN: Hostile-corpus builders -- same shapes as asset_decode_hostile_sanity.cpp's own (that file
//     is a different test binary with no shared header for these, so they are re-derived here;
//     see this file's top comment for why re-deriving is deliberate, not duplication-by-accident).
// PT: Construtores de corpus hostil -- mesmas formas dos próprios de asset_decode_hostile_
//     sanity.cpp (aquele é um binário de teste diferente, sem header compartilhado para isto,
//     então são re-derivados aqui; ver o comentário de topo deste arquivo pro porquê de
//     re-derivar ser deliberado, não duplicação por acidente).
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
  data.push_back(0); // compression method
  data.push_back(0); // filter method
  data.push_back(0); // interlace method

  std::vector<unsigned char> chunk;
  put_be32(chunk, static_cast<uint32_t>(data.size())); // == 13
  put_tag(chunk, "IHDR");
  chunk.insert(chunk.end(), data.begin(), data.end());
  put_be32(chunk, 0); // dummy CRC, stb_image never validates it
  return chunk;
}

// EN: PNG truncated mid-IDAT -- signature + valid 4x4 RGBA IHDR + an IDAT chunk header declaring
//     length=100 but only 10 bytes follow before EOF.
// PT: PNG truncado no meio do IDAT -- assinatura + IHDR válido 4x4 RGBA + header de chunk IDAT
//     declarando length=100 mas só 10 bytes seguem antes do EOF.
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

// EN: Pure garbage, matches no known image-format magic.
// PT: Lixo puro, não casa com nenhuma magic de formato de imagem conhecido.
std::vector<unsigned char> build_garbage() {
  std::vector<unsigned char> f;
  f.reserve(4096);
  for (int i = 0; i < 4096; ++i)
    f.push_back(static_cast<unsigned char>((i * 167 + 31) & 0xFF));
  return f;
}

// EN: Absurd declared dimensions (100000x100000 RGBA, ~40 GiB raw). No IDAT needed -- stb_image's
//     own overflow-safe size check inside IHDR parsing rejects this before any pixel allocation.
// PT: Dimensões declaradas absurdas (100000x100000 RGBA, ~40 GiB brutos). Sem IDAT necessário --
//     a própria guarda de overflow do stb_image dentro do parse do IHDR rejeita isto antes de
//     qualquer alocação de pixel.
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
  // EN: Group 1a -- valid PNG decode + premultiply verified BY VALUE (exact, lossless format).
  //     Pixel layout of image_decode_rgba_2x2.png (row-major, top-left origin, matching PNG's
  //     own raster order and this project's own coordinate convention -- see docs/draw2d.md's
  //     future D5 for where this convention is documented publicly):
  //       (x=0,y=0) RGBA(200,100,50,255) opaque      -> premultiply no-op: (200,100,50,255)
  //       (x=1,y=0) RGBA(200,100,50,  0) transparent -> premultiply zeroes RGB: (0,0,0,0)
  //                 (the exact halo-bug case image_decode.hpp's own top comment names)
  //       (x=0,y=1) RGBA( 10, 20, 30,128) half alpha -> hand-derived: 10*128/255=5 (1280/255,
  //                 rem 5), 20*128/255=10 (2560/255, rem 10), 30*128/255=15 (3840/255, rem 15)
  //                 -> (5,10,15,128)
  //       (x=1,y=1) RGBA(255,255,255,255) opaque white -> premultiply no-op: (255,255,255,255)
  // PT: Grupo 1a -- decode PNG válido + premultiply verificado POR VALOR (exato, formato sem
  //     perda). Layout de pixel de image_decode_rgba_2x2.png (linha-major, origem top-left,
  //     casando com a própria ordem de raster do PNG e a convenção de coordenadas deste projeto):
  //     ver os valores EN acima (espelhados aqui em conceito).
  // ===========================================================================================
  {
    const auto bytes = read_file("image_decode_rgba_2x2.png");
    check(!bytes.empty(), "png fixture: image_decode_rgba_2x2.png read (missing from build dir?)");

    const DecodedImage decoded = decode_premultiplied_rgba(bytes.data(), bytes.size());
    check(decoded.ok, "png fixture: decode_premultiplied_rgba reports ok == true");
    check(decoded.width == 2, "png fixture: width == 2");
    check(decoded.height == 2, "png fixture: height == 2");
    check(decoded.rgba.size() == 16, "png fixture: rgba buffer is width*height*4 == 16 bytes");

    if (decoded.ok && decoded.width == 2 && decoded.height == 2 && decoded.rgba.size() == 16) {
      const unsigned char* px = decoded.rgba.data();
      const unsigned char expect_opaque[4] = {200, 100, 50, 255};
      const unsigned char expect_zeroed[4] = {0, 0, 0, 0};
      const unsigned char expect_half[4] = {5, 10, 15, 128};
      const unsigned char expect_white[4] = {255, 255, 255, 255};

      auto pixel_matches = [&](int i, const unsigned char expect[4]) {
        return px[i * 4 + 0] == expect[0] && px[i * 4 + 1] == expect[1] &&
               px[i * 4 + 2] == expect[2] && px[i * 4 + 3] == expect[3];
      };
      check(pixel_matches(0, expect_opaque), "png fixture: (0,0) opaque pixel premultiply no-op");
      check(pixel_matches(1, expect_zeroed),
            "png fixture: (1,0) transparent pixel RGB zeroed by premultiply (halo-bug guard)");
      check(pixel_matches(2, expect_half), "png fixture: (0,1) half-alpha pixel matches hand-derived value");
      check(pixel_matches(3, expect_white), "png fixture: (1,1) opaque white pixel premultiply no-op");
    }
  }

  // ===========================================================================================
  // EN: Group 1b -- valid JPEG decode. Lossy format: RGB checked with tolerance, A checked exact
  //     (stb_image always reports 255 for a source format with no native alpha channel).
  // PT: Grupo 1b -- decode JPEG válido. Formato com perda: RGB checado com tolerância, A checado
  //     exato (stb_image sempre reporta 255 pra um formato de origem sem canal alpha nativo).
  // ===========================================================================================
  {
    const auto bytes = read_file("image_decode_solid.jpg");
    check(!bytes.empty(), "jpg fixture: image_decode_solid.jpg read (missing from build dir?)");

    const DecodedImage decoded = decode_premultiplied_rgba(bytes.data(), bytes.size());
    check(decoded.ok, "jpg fixture: decode_premultiplied_rgba reports ok == true");
    check(decoded.width == 4, "jpg fixture: width == 4");
    check(decoded.height == 4, "jpg fixture: height == 4");
    check(decoded.rgba.size() == 64, "jpg fixture: rgba buffer is width*height*4 == 64 bytes");

    if (decoded.ok && decoded.rgba.size() == 64) {
      const int expect_r = 100, expect_g = 150, expect_b = 200;
      const int tolerance = 12; // EN: JPEG is lossy even at quality 100. PT: JPEG tem perda mesmo em qualidade 100.
      bool rgb_ok = true, alpha_ok = true;
      for (size_t i = 0; i < 16; ++i) {
        const unsigned char* p = &decoded.rgba[i * 4];
        if (std::abs(static_cast<int>(p[0]) - expect_r) > tolerance ||
            std::abs(static_cast<int>(p[1]) - expect_g) > tolerance ||
            std::abs(static_cast<int>(p[2]) - expect_b) > tolerance)
          rgb_ok = false;
        if (p[3] != 255)
          alpha_ok = false;
      }
      check(rgb_ok, "jpg fixture: every pixel's RGB within tolerance of the source solid colour");
      check(alpha_ok, "jpg fixture: every pixel's A == 255 (no native alpha in JPEG)");
    }
  }

  // ===========================================================================================
  // EN: Group 2 -- hostile corpus reused directly at the helper seam. Every case here must
  //     return ok == false; none may crash/UB (run under ASan in the review/CI leg).
  // PT: Grupo 2 -- corpus hostil reusado direto na costura do helper. Todo caso aqui precisa
  //     retornar ok == false; nenhum pode crashar/UB (roda sob ASan na perna de review/CI).
  // ===========================================================================================
  {
    const auto bytes = build_png_truncated();
    check(!decode_premultiplied_rgba(bytes.data(), bytes.size()).ok,
          "hostile: png_truncated rejected (ok == false)");
  }
  {
    const auto bytes = build_garbage();
    check(!decode_premultiplied_rgba(bytes.data(), bytes.size()).ok,
          "hostile: garbage rejected (ok == false)");
  }
  {
    const auto bytes = build_huge_dims();
    check(!decode_premultiplied_rgba(bytes.data(), bytes.size()).ok,
          "hostile: huge_dims rejected (ok == false)");
  }
  {
    const std::vector<unsigned char> empty;
    check(!decode_premultiplied_rgba(empty.data(), empty.size()).ok,
          "hostile: zero-byte input rejected (ok == false)");
  }
  {
    // EN: The helper's OWN 256 MiB cap (kMaxImageDecodeBytes), discriminated independently of
    //     render_gl3.cpp's own pre-read file-size guard -- see this file's top comment. `data`
    //     is a REAL (tiny) buffer, but the claimed `len` is one byte past the cap; safe because
    //     the cap check runs before `data` is ever dereferenced (see image_decode.hpp's own
    //     guard order comment) -- no huge allocation, no out-of-bounds read, ever attempted.
    // PT: O teto PRÓPRIO do helper de 256 MiB (kMaxImageDecodeBytes), discriminado
    //     independentemente da própria guarda pré-leitura de tamanho de arquivo de
    //     render_gl3.cpp -- ver o comentário de topo deste arquivo. `data` é um buffer REAL
    //     (minúsculo), mas o `len` reivindicado é um byte além do teto; seguro porque a checagem
    //     do teto roda antes de `data` sequer ser dereferenciado (ver o próprio comentário de
    //     ordem de guarda de image_decode.hpp) -- nenhuma alocação enorme, nenhuma leitura fora
    //     dos limites, jamais tentada.
    const unsigned char tiny[4] = {0, 0, 0, 0};
    check(!decode_premultiplied_rgba(tiny, kMaxImageDecodeBytes + 1).ok,
          "hostile: len one byte past kMaxImageDecodeBytes rejected (ok == false)");
  }
  {
    check(!decode_premultiplied_rgba(nullptr, 0).ok, "hostile: null data + zero len rejected (ok == false)");
    const unsigned char tiny[4] = {0, 0, 0, 0};
    check(!decode_premultiplied_rgba(tiny, 0).ok, "hostile: zero len with non-null data rejected (ok == false)");
    check(!decode_premultiplied_rgba(nullptr, 4).ok, "hostile: null data with non-zero len rejected (ok == false)");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "image_decode_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("image_decode_sanity: PASS");
  return 0;
}
