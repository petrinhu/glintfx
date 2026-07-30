// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx/image.hpp's IMG-ENCODE pair (encode_image_memory()/
//     encode_image_file()), the sibling of image_sanity.cpp (which tests the DECODE pair) and of
//     image_decode_sanity.cpp (which tests the private decode seam). No RmlUi, no GL, no window,
//     no `Draw2d`/`App`/`UiLayer` instance -- both functions under test are free functions
//     callable headless. Registered UNCONDITIONALLY -- runs in BOTH build configurations, same
//     "no display to isolate here" reasoning image_sanity.cpp/image_decode_sanity.cpp already
//     state.
//
//     PER-FORMAT ORACLE, chosen to be the STRONGEST verification affordable without adding a new
//     dependency:
//       - PNG, TGA: this library's OWN `decode_image_memory()` (glintfx/image.hpp) already
//         decodes both (stb_image_impl.cpp compiles PNG/JPEG/TGA decode support in, see that
//         file's own STBI_ONLY_* defines) -- round-trip encode -> decode -> compare EVERY pixel
//         BYTE-EXACT (both formats are lossless).
//       - JPG: round-trip through the SAME `decode_image_memory()`, but tolerance-based on RGB
//         (JPEG is lossy even at quality 100, same declared-tolerance convention
//         image_decode_sanity.cpp's/image_sanity.cpp's own JPEG groups already use) and
//         exact-alpha (JPEG carries no native alpha; stb_image always reports A=255 for it).
//       - BMP: this library has no BMP DECODER (STBI_ONLY_PNG/JPEG/TGA in stb_image_impl.cpp
//         deliberately excludes it) -- verified instead by hand-decoding stb_image_write's own
//         FIXED, DOCUMENTED wire layout for a comp==4 write (`stbi_write_bmp_core`,
//         stb_image_write.h: a 14-byte file header + a 108-byte BITMAPV4HEADER using
//         BI_BITFIELDS at 32bpp with masks 0x00ff0000/0x0000ff00/0x000000ff/0xff000000 for
//         R/G/B/A respectively, rows stored BOTTOM-UP -- this is BMP's own long-standing public
//         wire format, not stb-internal knowledge). See `read_u32le()`/the BMP group below for
//         the exact offsets, verified byte-exact against the source pixel buffer.
//       - HDR: this library has no HDR decoder either, and stb_image_write's own RLE HDR
//         scanline encoding (used for width >= 8) is materially more complex to hand-decode than
//         the plain per-pixel path it uses for width < 8 (`stbiw__write_hdr_scanline`,
//         stb_image_write.h) -- this test deliberately uses a width-4 fixture to land on that
//         simpler, fully-specified Radiance RGBE-per-pixel path (4 raw bytes/pixel, no RLE
//         framing at all) and decodes it by hand (RGBE -> linear is Radiance's own decades-old
//         public format, ldexp(1, exponent-128-8) * mantissa), verified WITHIN TOLERANCE (this
//         format quantizes to an 8-bit mantissa, so exact equality is not the right oracle even
//         disregarding this module's own documented lossy RGBA8->float conversion).
//       - QOI: this library ships QOI ENCODE only (see `src/qoi_encode.hpp`'s own top comment
//         and `docs/capabilities.md`'s declared ceiling) -- there is no glintfx decoder to round-
//         trip through. This test instead carries its OWN small, independent, TEST-ONLY QOI
//         decoder (`decode_qoi_test_only()` below), written from the SAME public specification
//         `qoi_encode.hpp` was (qoiformat.org), and round-trips a deliberately varied pixel
//         sequence through it: a long run (70 identical pixels, split across the format's own
//         62-pixel run cap into two RUN chunks), a run ending EXACTLY at the last pixel of the
//         image (the encoder's own "flush at end-of-image" branch, not just "flush at 62"), a
//         repeated colour reused far apart (exercises QOI_OP_INDEX), small per-channel deltas
//         (QOI_OP_DIFF), a wider luma-biased delta (QOI_OP_LUMA), and alpha changes
//         (QOI_OP_RGBA) -- a byte-exact pixel match after this independent decode is a much
//         stronger correctness proof than hand-verifying a handful of encoded bytes would be
//         (two independently-written implementations of the same spec agreeing, pixel-for-pixel,
//         over a sequence exercising every chunk type).
//
//     Hostile corpus (TODO.md `IMG-ENCODE`'s own explicit list): null `pixels`, zero/negative
//     dimensions, a `width`/`height` pair whose byte count would overflow size_t arithmetic, a
//     `width`/`height` pair one step past `kMaxImageEncodeBytes` (attacked at the BOUNDARY with a
//     real oversized claim against a tiny real buffer -- safe because the cap check runs before
//     `pixels` is ever dereferenced, same technique image_sanity.cpp's own over-cap decode test
//     already uses), the SAME cap exercised with a REAL 64 MiB buffer that must still succeed
//     (this repository's own canonized lesson: a boundary test only proves the boundary if it
//     attacks the edge with real semantics, not an absurd value that never runs the real path),
//     an out-of-range `static_cast<ImageFormat>(n)`, and, for `encode_image_file()`: a null path,
//     an empty path, a path to an existing DIRECTORY, and a path whose parent directory does not
//     exist (stands in for "no permission" -- both degrade identically, to "cannot open for
//     writing", see image_encode.cpp's own top comment for why no separate stat() pre-check
//     exists). This repository's own canonized allowlist-not-blocklist lesson does not apply
//     here directly (there is no string/format field to filter) but its SPIRIT does: `ImageFormat`
//     is validated by an explicit `switch`/`default` in image_encode.cpp, not by checking against
//     a list of known-bad values.
// PT: Teste unit puro para o par IMG-ENCODE de glintfx/image.hpp (encode_image_memory()/
//     encode_image_file()), irmão do image_sanity.cpp (que testa o par DECODE) e do
//     image_decode_sanity.cpp (que testa a costura privada de decode). Sem RmlUi, sem GL, sem
//     janela, sem instância `Draw2d`/`App`/`UiLayer` -- as duas funções sob teste são free
//     functions chamáveis headless. Registrado INCONDICIONALMENTE -- roda nas DUAS configurações
//     de build, mesma racional "nada relacionado a display pra isolar aqui" que
//     image_sanity.cpp/image_decode_sanity.cpp já declaram.
//
//     ORÁCULO POR-FORMATO, escolhido pra ser a verificação MAIS FORTE que cabe sem somar
//     dependência nova: ver o EN acima pro detalhe completo por formato (PNG/TGA via a própria
//     decode_image_memory(), byte-exato; JPG idem com tolerância; BMP decodificado à mão contra o
//     layout de wire fixo e documentado do próprio stb_image_write; HDR decodificado à mão contra
//     o caminho RGBE-por-pixel simples do Radiance, com tolerância; QOI via um decoder próprio
//     SÓ-DE-TESTE, escrito da MESMA especificação pública que qoi_encode.hpp, round-trip byte-
//     exato sobre uma sequência de pixel deliberadamente variada).
//
//     Corpus hostil (a própria lista explícita do IMG-ENCODE no TODO.md): `pixels` nulo,
//     dimensões zero/negativas, um par `width`/`height` que daria overflow em aritmética size_t,
//     um par `width`/`height` um passo além de `kMaxImageEncodeBytes` (atacado na FRONTEIRA com
//     uma reivindicação real acima do teto contra um buffer real minúsculo), o MESMO teto
//     exercitado com um buffer REAL de 64 MiB que ainda precisa ter sucesso, um
//     `static_cast<ImageFormat>(n)` fora de faixa, e, pro `encode_image_file()`: um path nulo,
//     vazio, um path pra um DIRETÓRIO existente, e um path cujo diretório pai não existe (fica no
//     lugar de "sem permissão" -- os dois degradam identicamente).
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

std::uint32_t read_u32le(const unsigned char* p) {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint16_t read_u16le(const unsigned char* p) {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

// EN: Big-endian 32-bit reader -- QOI's own header fields (width/height) are big-endian per
//     spec, unlike BMP's little-endian fields read by read_u32le() above.
// PT: Leitor de 32 bits big-endian -- os próprios campos de cabeçalho do QOI (width/height) são
//     big-endian pela spec, diferente dos campos little-endian do BMP lidos por read_u32le() acima.
std::uint32_t read_u32be(const unsigned char* p) {
  return (static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16) |
         (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}

// ---------------------------------------------------------------------------
// EN: A small deterministic pixel-buffer generator (NOT random -- a fixed formula so the test is
//     reproducible byte-for-byte across runs/platforms), used by several groups below to build
//     RGBA8 straight-alpha fixtures without hand-authoring every byte.
// PT: Um gerador de buffer de pixel pequeno e determinístico (NÃO aleatório -- uma fórmula fixa
//     pra que o teste seja reproduzível byte-a-byte entre execuções/plataformas), usado por vários
//     grupos abaixo pra construir fixtures RGBA8 alpha straight sem autorar cada byte à mão.
std::vector<unsigned char> make_gradient(int width, int height) {
  std::vector<unsigned char> px(static_cast<std::size_t>(width) * height * 4);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 4;
      px[i + 0] = static_cast<unsigned char>((x * 37 + y * 11) & 0xFF);
      px[i + 1] = static_cast<unsigned char>((x * 53 + y * 29) & 0xFF);
      px[i + 2] = static_cast<unsigned char>((x * 17 + y * 61) & 0xFF);
      px[i + 3] = static_cast<unsigned char>((x + y) % 2 == 0 ? 255 : 128); // varied, never fully opaque-only.
    }
  }
  return px;
}

// ---------------------------------------------------------------------------
// EN: TEST-ONLY QOI decoder (see this file's own top comment for why this exists and why it is
//     legitimate as an oracle: independently written from the same public spec `qoi_encode.hpp`
//     is, never copied from either that file or the public-domain reference `qoi.h`). NOT part
//     of the library -- this library ships QOI ENCODE only.
// PT: Decoder QOI SÓ-DE-TESTE (ver o próprio comentário de topo deste arquivo pro porquê disto
//     existir e ser legítimo como oráculo: escrito de forma independente a partir da mesma spec
//     pública que qoi_encode.hpp, nunca copiado daquele arquivo nem do `qoi.h` de referência de
//     domínio público). NÃO faz parte da biblioteca -- esta biblioteca embarca só ENCODE de QOI.
// ---------------------------------------------------------------------------
struct QoiPx {
  unsigned char r = 0, g = 0, b = 0, a = 0;
  bool operator==(const QoiPx& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
};

std::vector<unsigned char> decode_qoi_test_only(const std::vector<unsigned char>& bytes,
                                                int expect_width, int expect_height) {
  std::vector<unsigned char> out;
  if (bytes.size() < 14)
    return out; // too short for even the header.
  if (bytes[0] != 'q' || bytes[1] != 'o' || bytes[2] != 'i' || bytes[3] != 'f')
    return out;
  const std::uint32_t w = read_u32be(bytes.data() + 4);
  const std::uint32_t h = read_u32be(bytes.data() + 8);
  if (static_cast<int>(w) != expect_width || static_cast<int>(h) != expect_height)
    return out;

  const std::size_t pixel_count = static_cast<std::size_t>(w) * h;
  out.reserve(pixel_count * 4);

  QoiPx cache[64] = {};
  QoiPx prev{0, 0, 0, 255};
  std::size_t pos = 14;
  std::size_t produced = 0;

  while (produced < pixel_count && pos < bytes.size()) {
    const unsigned char tag = bytes[pos++];
    QoiPx cur = prev;
    std::size_t run_len = 1;

    if (tag == 0xFF) { // QOI_OP_RGBA
      cur.r = bytes[pos + 0];
      cur.g = bytes[pos + 1];
      cur.b = bytes[pos + 2];
      cur.a = bytes[pos + 3];
      pos += 4;
    } else if (tag == 0xFE) { // QOI_OP_RGB
      cur.r = bytes[pos + 0];
      cur.g = bytes[pos + 1];
      cur.b = bytes[pos + 2];
      pos += 3;
    } else if ((tag >> 6) == 0) { // QOI_OP_INDEX
      cur = cache[tag & 0x3F];
    } else if ((tag >> 6) == 1) { // QOI_OP_DIFF
      const int dr = ((tag >> 4) & 0x3) - 2;
      const int dg = ((tag >> 2) & 0x3) - 2;
      const int db = (tag & 0x3) - 2;
      cur.r = static_cast<unsigned char>(prev.r + dr);
      cur.g = static_cast<unsigned char>(prev.g + dg);
      cur.b = static_cast<unsigned char>(prev.b + db);
    } else if ((tag >> 6) == 2) { // QOI_OP_LUMA
      const int dg = (tag & 0x3F) - 32;
      const unsigned char b2 = bytes[pos++];
      const int dr = ((b2 >> 4) & 0xF) - 8 + dg;
      const int db = (b2 & 0xF) - 8 + dg;
      cur.r = static_cast<unsigned char>(prev.r + dr);
      cur.g = static_cast<unsigned char>(prev.g + dg);
      cur.b = static_cast<unsigned char>(prev.b + db);
    } else { // QOI_OP_RUN (tag>>6 == 3)
      run_len = static_cast<std::size_t>(tag & 0x3F) + 1;
      cur = prev;
    }

    for (std::size_t k = 0; k < run_len && produced < pixel_count; ++k, ++produced) {
      out.push_back(cur.r);
      out.push_back(cur.g);
      out.push_back(cur.b);
      out.push_back(cur.a);
    }
    cache[(cur.r * 3u + cur.g * 5u + cur.b * 7u + cur.a * 11u) & 63u] = cur;
    prev = cur;
  }

  if (produced != pixel_count)
    out.clear();
  return out;
}

} // namespace

using namespace glintfx;

int main() {
  // ===========================================================================================
  // EN: Group 1 -- PNG round-trip, byte-exact, through this library's OWN decode_image_memory().
  //     4x3 gradient fixture with a mix of alpha values (128/255) -- a fully-opaque-only fixture
  //     would not discriminate a correct alpha channel from a broken one (this repository's own
  //     canonized "opaque fixture is a blind fixture" lesson).
  // PT: Grupo 1 -- round-trip PNG, byte-exato, pela PRÓPRIA decode_image_memory() desta
  //     biblioteca. Fixture gradiente 4x3 com mistura de valores de alpha (128/255) -- uma
  //     fixture só-opaca não discriminaria um canal alpha correto de um quebrado.
  // ===========================================================================================
  {
    const int w = 4, h = 3;
    const auto src = make_gradient(w, h);
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Png, w, h, src.data());
    check(enc.ok, "png: encode_image_memory reports ok == true");
    check(!enc.bytes.empty(), "png: encoded buffer is non-empty");
    check(enc.bytes.size() >= 8 && enc.bytes[0] == 0x89 && enc.bytes[1] == 'P' && enc.bytes[2] == 'N' &&
              enc.bytes[3] == 'G',
          "png: output starts with the PNG magic signature");

    const DecodedImagePixels dec = decode_image_memory(enc.bytes.data(), enc.bytes.size());
    check(dec.ok, "png: round-trip decode_image_memory reports ok == true");
    check(dec.width == w && dec.height == h, "png: round-trip dimensions match");
    check(dec.pixels == src, "png: round-trip pixels are BYTE-EXACT (lossless format)");
  }

  // ===========================================================================================
  // EN: Group 2 -- TGA round-trip, byte-exact, same oracle as PNG above (TGA is also lossless).
  // PT: Grupo 2 -- round-trip TGA, byte-exato, mesmo oráculo do PNG acima (TGA também é sem
  //     perda).
  // ===========================================================================================
  {
    const int w = 5, h = 2;
    const auto src = make_gradient(w, h);
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Tga, w, h, src.data());
    check(enc.ok, "tga: encode_image_memory reports ok == true");
    check(!enc.bytes.empty(), "tga: encoded buffer is non-empty");

    const DecodedImagePixels dec = decode_image_memory(enc.bytes.data(), enc.bytes.size());
    check(dec.ok, "tga: round-trip decode_image_memory reports ok == true");
    check(dec.width == w && dec.height == h, "tga: round-trip dimensions match");
    check(dec.pixels == src, "tga: round-trip pixels are BYTE-EXACT (lossless format)");
  }

  // ===========================================================================================
  // EN: Group 3 -- JPG round-trip, tolerance-based RGB, exact alpha (lossy format, no native
  //     alpha channel -- same declared-tolerance convention image_sanity.cpp's own JPEG group
  //     already uses). A SOLID colour fixture (not a gradient) keeps the tolerance check simple
  //     and avoids DCT block-edge artifacts a gradient could introduce.
  // PT: Grupo 3 -- round-trip JPG, RGB com tolerância, alpha exato (formato com perda, sem canal
  //     alpha nativo). Fixture de cor SÓLIDA (não gradiente) mantém a checagem de tolerância
  //     simples e evita artefatos de borda de bloco DCT que um gradiente poderia introduzir.
  // ===========================================================================================
  {
    const int w = 8, h = 8;
    std::vector<unsigned char> src(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < src.size(); i += 4) {
      src[i + 0] = 90;
      src[i + 1] = 140;
      src[i + 2] = 210;
      src[i + 3] = 255; // opaque -- JPG has no alpha to lose, this is the honest source value.
    }
    EncodeImageOptions opts;
    opts.jpg_quality = 95;
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Jpg, w, h, src.data(), opts);
    check(enc.ok, "jpg: encode_image_memory reports ok == true");
    check(enc.bytes.size() >= 2 && enc.bytes[0] == 0xFF && enc.bytes[1] == 0xD8,
          "jpg: output starts with the JPEG SOI marker (0xFFD8)");

    const DecodedImagePixels dec = decode_image_memory(enc.bytes.data(), enc.bytes.size());
    check(dec.ok, "jpg: round-trip decode_image_memory reports ok == true");
    check(dec.width == w && dec.height == h, "jpg: round-trip dimensions match");
    if (dec.ok && dec.pixels.size() == src.size()) {
      const int tolerance = 12;
      bool rgb_ok = true, alpha_ok = true;
      for (std::size_t i = 0; i < dec.pixels.size(); i += 4) {
        if (std::abs(static_cast<int>(dec.pixels[i + 0]) - 90) > tolerance ||
            std::abs(static_cast<int>(dec.pixels[i + 1]) - 140) > tolerance ||
            std::abs(static_cast<int>(dec.pixels[i + 2]) - 210) > tolerance)
          rgb_ok = false;
        if (dec.pixels[i + 3] != 255)
          alpha_ok = false;
      }
      check(rgb_ok, "jpg: round-trip RGB within tolerance of the source solid colour");
      check(alpha_ok, "jpg: round-trip alpha == 255 (JPEG carries no native alpha)");
    }
  }

  // ===========================================================================================
  // EN: Group 4 -- BMP, hand-decoded against stb_image_write's own fixed comp==4 wire layout
  //     (14-byte file header + 108-byte BITMAPV4HEADER, BI_BITFIELDS 32bpp, rows bottom-up --
  //     see this file's own top comment for the exact citation). Verified BYTE-EXACT, including
  //     alpha (this is the one format group above that could not use decode_image_memory() as
  //     its oracle -- see this file's top comment for why).
  // PT: Grupo 4 -- BMP, decodificado à mão contra o próprio layout de wire fixo de comp==4 do
  //     stb_image_write. Verificado BYTE-EXATO, alpha incluso.
  // ===========================================================================================
  {
    const int w = 3, h = 2;
    const auto src = make_gradient(w, h);
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Bmp, w, h, src.data());
    check(enc.ok, "bmp: encode_image_memory reports ok == true");

    const std::size_t expect_size = 14 + 108 + static_cast<std::size_t>(w) * h * 4;
    check(enc.bytes.size() == expect_size, "bmp: output size matches 14+108+w*h*4 exactly (no row padding at 32bpp)");

    if (enc.bytes.size() == expect_size) {
      const unsigned char* d = enc.bytes.data();
      check(d[0] == 'B' && d[1] == 'M', "bmp: file header magic 'BM'");
      check(read_u32le(d + 2) == static_cast<std::uint32_t>(expect_size), "bmp: file-size field matches actual size");
      check(read_u32le(d + 18) == static_cast<std::uint32_t>(w), "bmp: width field matches");
      check(static_cast<std::int32_t>(read_u32le(d + 22)) == h, "bmp: height field matches");
      check(read_u16le(d + 28) == 32, "bmp: bits-per-pixel field is 32");

      const unsigned char* pixel_data = d + 14 + 108;
      bool exact = true;
      for (int row = 0; row < h; ++row) {
        // EN: BMP rows are bottom-up: file row 0 == source row (h-1-0).
        // PT: Linhas do BMP são bottom-up: linha 0 do arquivo == linha de origem (h-1-0).
        const int src_row = h - 1 - row;
        for (int col = 0; col < w; ++col) {
          const unsigned char* px = pixel_data + (static_cast<std::size_t>(row) * w + col) * 4;
          const std::size_t src_i = (static_cast<std::size_t>(src_row) * w + col) * 4;
          // EN: BI_BITFIELDS masks 0x00ff0000/0x0000ff00/0x000000ff/0xff000000 for R/G/B/A on a
          //     little-endian DWORD -> byte order in the stream is B,G,R,A.
          // PT: Máscaras BI_BITFIELDS 0x00ff0000/0x0000ff00/0x000000ff/0xff000000 pra R/G/B/A num
          //     DWORD little-endian -> ordem de byte no stream é B,G,R,A.
          if (px[0] != src[src_i + 2] || px[1] != src[src_i + 1] || px[2] != src[src_i + 0] ||
              px[3] != src[src_i + 3])
            exact = false;
        }
      }
      check(exact, "bmp: every pixel round-trips byte-exact (B,G,R,A order, bottom-up rows)");
    }
  }

  // ===========================================================================================
  // EN: Group 5 -- HDR, hand-decoded against the Radiance RGBE-per-pixel path (width < 8, no RLE
  //     framing -- see this file's own top comment). Tolerance-based (8-bit RGBE mantissa
  //     quantization, plus this module's own documented RGBA8->float lossy conversion).
  // PT: Grupo 5 -- HDR, decodificado à mão contra o caminho RGBE-por-pixel do Radiance (width <
  //     8, sem RLE). Com tolerância (quantização de mantissa RGBE de 8 bits, mais a própria
  //     conversão RGBA8->float com perda deste módulo).
  // ===========================================================================================
  {
    const int w = 4, h = 1; // width < 8 forces the simple per-pixel RGBE path, no RLE framing.
    const unsigned char src[16] = {
        0,
        0,
        0,
        255, // black
        255,
        255,
        255,
        255, // white
        255,
        0,
        0,
        255, // pure red
        128,
        128,
        128,
        255, // mid-grey
    };
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Hdr, w, h, src);
    check(enc.ok, "hdr: encode_image_memory reports ok == true");
    check(enc.bytes.size() >= 11 && std::memcmp(enc.bytes.data(), "#?RADIANCE\n", 11) == 0,
          "hdr: output starts with the Radiance magic '#?RADIANCE'");

    // EN: The simple (non-RLE) per-pixel path writes EXACTLY width*height*4 raw bytes with
    //     nothing after them -- so the pixel data is unambiguously the LAST 16 bytes of the
    //     output, regardless of the exact length of the ASCII header text before it.
    // PT: O caminho simples (sem RLE) por-pixel escreve EXATAMENTE width*height*4 bytes crus sem
    //     nada depois -- então o dado de pixel é inequivocamente os ÚLTIMOS 16 bytes da saída,
    //     independente do comprimento exato do texto ASCII do cabeçalho antes dele.
    const std::size_t pixel_bytes = static_cast<std::size_t>(w) * h * 4;
    check(enc.bytes.size() > pixel_bytes, "hdr: output is longer than just the pixel payload (has a header)");
    if (enc.bytes.size() > pixel_bytes) {
      const unsigned char* rgbe = enc.bytes.data() + (enc.bytes.size() - pixel_bytes);
      auto decode_rgbe = [](const unsigned char* px, float out3[3]) {
        if (px[3] == 0) {
          out3[0] = out3[1] = out3[2] = 0.0f;
          return;
        }
        const float scale = std::ldexp(1.0f, static_cast<int>(px[3]) - 128 - 8);
        out3[0] = (px[0] + 0.5f) * scale;
        out3[1] = (px[1] + 0.5f) * scale;
        out3[2] = (px[2] + 0.5f) * scale;
      };
      const float expect[4][3] = {
          {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f}};
      const float tolerance = 0.06f; // 8-bit RGBE mantissa + our own /255 conversion, both lossy.
      bool ok = true;
      for (int i = 0; i < 4; ++i) {
        float got[3];
        decode_rgbe(rgbe + i * 4, got);
        for (int c = 0; c < 3; ++c) {
          if (std::fabs(got[c] - expect[i][c]) > tolerance)
            ok = false;
        }
      }
      check(ok, "hdr: every pixel's RGBE-decoded linear value is within tolerance of the source /255 value");
    }
  }

  // ===========================================================================================
  // EN: Group 6 -- PPM, byte-exact hand-check (this module's own hand-written P6 writer, no
  //     stb_image_write involved -- see image_encode.cpp's own PPM case comment).
  // PT: Grupo 6 -- PPM, checagem byte-exata à mão (o próprio escritor P6 escrito à mão deste
  //     módulo, sem stb_image_write envolvido).
  // ===========================================================================================
  {
    const int w = 2, h = 2;
    const auto src = make_gradient(w, h);
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Ppm, w, h, src.data());
    check(enc.ok, "ppm: encode_image_memory reports ok == true");
    const std::string expect_header = "P6\n2 2\n255\n";
    check(enc.bytes.size() >= expect_header.size() &&
              std::memcmp(enc.bytes.data(), expect_header.data(), expect_header.size()) == 0,
          "ppm: header is exactly 'P6\\n2 2\\n255\\n'");
    const std::size_t payload_off = expect_header.size();
    check(enc.bytes.size() == payload_off + static_cast<std::size_t>(w) * h * 3,
          "ppm: total size is header + w*h*3 (no alpha byte)");
    if (enc.bytes.size() == payload_off + static_cast<std::size_t>(w) * h * 3) {
      bool exact = true;
      for (int i = 0; i < w * h; ++i) {
        if (enc.bytes[payload_off + i * 3 + 0] != src[i * 4 + 0] ||
            enc.bytes[payload_off + i * 3 + 1] != src[i * 4 + 1] ||
            enc.bytes[payload_off + i * 3 + 2] != src[i * 4 + 2])
          exact = false;
      }
      check(exact, "ppm: every pixel's RGB round-trips byte-exact, alpha dropped");
    }
  }

  // ===========================================================================================
  // EN: Group 7 -- QOI, round-trip through this file's own TEST-ONLY decoder (see this file's
  //     top comment). Sequence deliberately exercises: a 70-pixel run (splits into a 62-run +
  //     an 8-run, QOI's own per-chunk cap), a handful of small distinct pixels (RGB/DIFF/LUMA),
  //     a colour reused far apart (QOI_OP_INDEX), an alpha CHANGE (QOI_OP_RGBA), and a trailing
  //     run that ends EXACTLY at the image's last pixel (the encoder's "flush at end-of-image"
  //     branch, distinct from "flush at 62").
  // PT: Grupo 7 -- QOI, round-trip pelo próprio decoder SÓ-DE-TESTE deste arquivo. Sequência
  //     exercita de propósito: uma run de 70 pixels (divide em run-de-62 + run-de-8, o próprio
  //     teto por-chunk do QOI), um punhado de pixels pequenos distintos (RGB/DIFF/LUMA), uma cor
  //     reusada de longe (QOI_OP_INDEX), uma MUDANÇA de alpha (QOI_OP_RGBA), e uma run final que
  //     termina EXATAMENTE no último pixel da imagem.
  // ===========================================================================================
  {
    std::vector<unsigned char> src;
    auto push_px = [&](unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
      src.push_back(r);
      src.push_back(g);
      src.push_back(b);
      src.push_back(a);
    };
    for (int i = 0; i < 70; ++i)
      push_px(5, 5, 5, 255);   // long run -> splits into 62 + 8.
    push_px(10, 20, 30, 255);  // fresh colour, cached at its own hash slot.
    push_px(11, 19, 30, 255);  // small delta from the previous pixel (QOI_OP_DIFF range).
    push_px(24, 29, 37, 255);  // wider delta (QOI_OP_LUMA range).
    push_px(10, 20, 30, 255);  // re-uses the colour pushed 3 pixels ago (QOI_OP_INDEX, if the
                               // cache slot survived -- correctness does not depend on which
                               // opcode the encoder actually chose, only on the decode matching).
    push_px(10, 20, 30, 0);    // SAME rgb, alpha changed -> forces QOI_OP_RGBA.
    push_px(200, 60, 90, 128); // another fresh colour with partial alpha.
    for (int i = 0; i < 5; ++i)
      push_px(200, 60, 90, 128); // trailing run that ends EXACTLY at the last pixel of the image.

    const int w = static_cast<int>(src.size() / 4), h = 1;
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Qoi, w, h, src.data());
    check(enc.ok, "qoi: encode_image_memory reports ok == true");
    check(enc.bytes.size() >= 4 && enc.bytes[0] == 'q' && enc.bytes[1] == 'o' && enc.bytes[2] == 'i' &&
              enc.bytes[3] == 'f',
          "qoi: output starts with the 'qoif' magic");
    check(enc.bytes.size() >= 8 &&
              enc.bytes[enc.bytes.size() - 8] == 0 && enc.bytes[enc.bytes.size() - 7] == 0 &&
              enc.bytes[enc.bytes.size() - 6] == 0 && enc.bytes[enc.bytes.size() - 5] == 0 &&
              enc.bytes[enc.bytes.size() - 4] == 0 && enc.bytes[enc.bytes.size() - 3] == 0 &&
              enc.bytes[enc.bytes.size() - 2] == 0 && enc.bytes[enc.bytes.size() - 1] == 1,
          "qoi: output ends with the spec-fixed 7x0x00 + 0x01 end marker");

    const std::vector<unsigned char> dec = decode_qoi_test_only(enc.bytes, w, h);
    check(dec == src,
          "qoi: round-trip through an independent test-only decoder is BYTE-EXACT "
          "(long run split, index reuse, diff, luma, and alpha-change chunks all exercised)");
  }

  // ===========================================================================================
  // EN: Group 8 -- the 64 MiB `kMaxImageEncodeBytes` boundary, attacked at the EDGE with REAL
  //     buffers on both sides (this repository's own canonized lesson: a boundary test only
  //     proves the boundary if it does this, not if it uses an absurd value that never runs the
  //     real code path). 4096x4096 RGBA8 == exactly 64 MiB -- MUST succeed (cheap format, PPM,
  //     chosen so the real-buffer success case stays fast); one pixel taller (4096x4097, ~16 KiB
  //     over the cap) MUST fail, exercised with a TINY real backing buffer (safe because the cap
  //     check runs before `pixels` is ever dereferenced past index 0 -- image_sanity.cpp's own
  //     over-cap decode test uses the identical technique).
  // PT: Grupo 8 -- a fronteira de 64 MiB do `kMaxImageEncodeBytes`, atacada na BEIRA com buffers
  //     REAIS dos dois lados. 4096x4096 RGBA8 == exatamente 64 MiB -- PRECISA ter sucesso; um
  //     pixel mais alto (4096x4097, ~16 KiB acima do teto) PRECISA falhar, exercitado com um
  //     buffer real MINÚSCULO (seguro porque a checagem do teto roda antes de `pixels` ser
  //     dereferenciado além do índice 0).
  // ===========================================================================================
  {
    const int w = 4096, h = 4096; // 4096*4096*4 == 67108864 == 64 MiB exactly.
    std::vector<unsigned char> big(static_cast<std::size_t>(w) * h * 4, 0);
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Ppm, w, h, big.data());
    check(enc.ok, "boundary: exactly-64-MiB input (4096x4096) succeeds with a REAL buffer");
    if (enc.ok) {
      const std::string expect_header = "P6\n4096 4096\n255\n";
      check(enc.bytes.size() == expect_header.size() + static_cast<std::size_t>(w) * h * 3,
            "boundary: at-cap output size is header + w*h*3");
    }
  }
  {
    const unsigned char tiny[4] = {0, 0, 0, 0};
    const EncodedImageBytes enc = encode_image_memory(ImageFormat::Ppm, 4096, 4097, tiny);
    check(!enc.ok, "boundary: one row past the 64 MiB cap (4096x4097) rejected (ok == false)");
    check(enc.bytes.empty(), "boundary: rejected encode leaves bytes empty");
  }

  // ===========================================================================================
  // EN: Group 9 -- hostile corpus: null pixels, zero/negative dimensions, size-overflowing
  //     dimensions, an out-of-range ImageFormat cast.
  // PT: Grupo 9 -- corpus hostil: pixels nulo, dimensões zero/negativas, dimensões que dão
  //     overflow de tamanho, um cast de ImageFormat fora de faixa.
  // ===========================================================================================
  {
    const unsigned char tiny[4] = {1, 2, 3, 4};
    check(!encode_image_memory(ImageFormat::Png, 4, 4, nullptr).ok, "hostile: null pixels rejected");
    check(!encode_image_memory(ImageFormat::Png, 0, 4, tiny).ok, "hostile: width == 0 rejected");
    check(!encode_image_memory(ImageFormat::Png, 4, 0, tiny).ok, "hostile: height == 0 rejected");
    check(!encode_image_memory(ImageFormat::Png, -1, 4, tiny).ok, "hostile: negative width rejected");
    check(!encode_image_memory(ImageFormat::Png, 4, -1, tiny).ok, "hostile: negative height rejected");
    check(!encode_image_memory(ImageFormat::Png, 100000, 100000, tiny).ok,
          "hostile: huge dims (100000x100000, far past kMaxImageEncodeBytes) rejected (ok == false, no crash)");
    check(!encode_image_memory(static_cast<ImageFormat>(999), 4, 4, tiny).ok,
          "hostile: out-of-range ImageFormat cast rejected (ok == false, no fall-through)");
    check(!encode_image_memory(static_cast<ImageFormat>(-1), 4, 4, tiny).ok,
          "hostile: negative ImageFormat cast rejected (ok == false)");

    // EN: A real call still works right after the corpus (same "still usable" oracle idiom
    //     image_sanity.cpp's own hostile groups use) -- `tiny` above is 1 pixel's worth of
    //     bytes, so the real call below deliberately requests a 1x1 image (matching `tiny`'s own
    //     actual size), never a larger one: this API has no way to validate that `pixels` truly
    //     backs `width * height * 4` bytes (the same raw-pointer-plus-dimensions trust contract
    //     `Draw2d::create_texture()` already documents, `draw2d.hpp`), so a "still usable" check
    //     here must stay within the buffer it actually has, not invite an out-of-bounds read of
    //     its own making.
    // PT: Uma chamada real ainda funciona logo após o corpus (mesmo idioma-oráculo "ainda
    //     usável" que os próprios grupos hostis de image_sanity.cpp usam) -- `tiny` acima vale 1
    //     pixel, então a chamada real abaixo pede de propósito uma imagem 1x1 (batendo com o
    //     próprio tamanho real de `tiny`), nunca uma maior: esta API não tem como validar que
    //     `pixels` realmente sustenta `width * height * 4` bytes (o mesmo contrato de confiança
    //     ponteiro-cru-mais-dimensões que o próprio `Draw2d::create_texture()` já documenta,
    //     `draw2d.hpp`), então uma checagem "ainda usável" aqui precisa ficar dentro do buffer
    //     que de fato tem, não convidar uma leitura fora-dos-limites da própria lavra.
    const EncodedImageBytes real = encode_image_memory(ImageFormat::Png, 1, 1, tiny);
    check(real.ok, "hostile: a real 1x1 encode_image_memory() call still works right after the corpus");
  }

  // ===========================================================================================
  // EN: Group 10 -- encode_image_file() hostile corpus: null path, empty path, a path to an
  //     existing directory, a path whose parent directory does not exist. Also: a real
  //     encode_image_file() call still works right after the corpus, and round-trips through
  //     decode_image_file() (PNG).
  // PT: Grupo 10 -- corpus hostil de encode_image_file(): path nulo, vazio, um path pra um
  //     diretório existente, um path cujo pai não existe. Também: uma chamada real
  //     encode_image_file() ainda funciona logo após o corpus, e faz round-trip por
  //     decode_image_file() (PNG).
  // ===========================================================================================
  {
    const auto src = make_gradient(2, 2);
    check(!encode_image_file(nullptr, ImageFormat::Png, 2, 2, src.data()), "hostile (file): null path rejected");
    check(!encode_image_file("", ImageFormat::Png, 2, 2, src.data()), "hostile (file): empty path rejected");
    check(!encode_image_file(".", ImageFormat::Png, 2, 2, src.data()),
          "hostile (file): path to an existing directory rejected");
    check(!encode_image_file("this/dir/does/not/exist/out.png", ImageFormat::Png, 2, 2, src.data()),
          "hostile (file): path whose parent directory does not exist rejected");

    const char* const out_path = "image_encode_sanity_out.png";
    check(encode_image_file(out_path, ImageFormat::Png, 2, 2, src.data()),
          "hostile (file): a real encode_image_file() call still works right after the corpus");
    const DecodedImagePixels dec = decode_image_file(out_path);
    check(dec.ok && dec.width == 2 && dec.height == 2 && dec.pixels == src,
          "hostile (file): the real file just written round-trips through decode_image_file()");
    std::remove(out_path);
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "image_encode_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("image_encode_sanity: PASS");
  return 0;
}
