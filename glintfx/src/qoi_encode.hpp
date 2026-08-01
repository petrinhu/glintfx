// SPDX-License-Identifier: Apache-2.0
// EN: IMG-ENCODE -- a clean-room QOI (Quite OK Image, qoiformat.org) encoder, written from the
//     PUBLIC SPECIFICATION alone (the QOI format is deliberately a few-hundred-line format
//     designed to be re-derived from its own spec document, not reverse-engineered from a
//     reference decoder) -- no `qoi.h` (the public-domain reference implementation) was read,
//     copied from, or consulted while writing this file, per this repository's clean-room
//     discipline (CLAUDE.md: "ao reimplementar/RE de libs externas, não copiar código --
//     reimplementar clean-room"). This is the ONE format in the IMG-ENCODE set that needed an
//     actual implementation: the other six (PNG/JPG/BMP/TGA/HDR via vendored stb_image_write,
//     PPM hand-written in image_encode.cpp) all had an existing encoder to call. QOI was picked
//     for internalization specifically because it is small, stable, and publicly documented --
//     it fits this repository's long-term clean-room trajectory (see CLAUDE.md's Camada 0
//     section) far better than PNG's deflate/zlib would.
//
//     PURE, TOTAL, HEADER-ONLY (same "pure seam" discipline as `image_decode.hpp`): no I/O, no
//     allocation beyond the caller-provided `out` vector's own growth, no logging (this header
//     cannot reach `glintfx::log()` and does not need to -- see `image_encode.cpp`'s own
//     doc-comment for where the guard/log split lives). `qoi_encode_rgba()` NEVER fails: given
//     `width > 0`, `height > 0`, and `pixels` pointing at `width * height * 4` valid bytes
//     (straight, non-premultiplied RGBA8 -- the SAME convention `DecodedImagePixels`/
//     `encode_image_memory()` already use, see `glintfx/include/glintfx/image.hpp`), it always
//     produces a well-formed QOI byte stream. Guarding `width`/`height`/`pixels` themselves (the
//     hostile-input surface: null pointer, zero/negative dimension, dimension overflow) is the
//     CALLER's job (`image_encode.cpp`), same division of labour `image_decode.hpp`'s own
//     `premultiply_rgba_inplace()`/`compute_content_bbox()` already establish between a pure
//     total helper and its guarded caller.
//
//     FORMAT LAYOUT (qoiformat.org, "The QOI File Format Specification"): a 14-byte header
//     (magic "qoif", width/height as 32-bit BIG-ENDIAN, 1 byte channel count, 1 byte colorspace
//     -- BOTH the last two fields are purely INFORMATIVE, a decoder is not required to act on
//     them, so this encoder always writes 4/0 regardless of whether the source alpha channel is
//     ever non-255), a stream of variable-length pixel-difference chunks, and an 8-byte end
//     marker (seven 0x00 bytes followed by one 0x01 byte). The encoder below is a literal,
//     unabridged implementation of the six chunk types the spec defines, chosen greedily in the
//     spec's own priority order (a byte-identical run collapses into QOI_OP_RUN; otherwise a
//     64-entry rolling colour cache is checked (QOI_OP_INDEX); otherwise, if alpha is unchanged
//     from the previous pixel, a small RGB delta is tried as QOI_OP_DIFF, then a wider
//     luma-biased delta as QOI_OP_LUMA; failing all of those, the full pixel is written verbatim
//     as QOI_OP_RGB (alpha unchanged) or QOI_OP_RGBA (alpha changed)) -- see the inline comments
//     at each branch below for the exact bit layout, cited against the spec's own tag values.
//
//     WHY A HAND-ROLLED TEST-ONLY DECODER, NOT A SHIPPED ONE, VERIFIES THIS (see this repo's own
//     `tests/image_encode_sanity.cpp`): the round-trip oracle -- decode what this encoder just
//     produced and diff every pixel against the source -- is a MUCH stronger correctness proof
//     than hand-verifying a handful of encoded bytes (this file's own author's note: writing the
//     decoder from the SAME spec, independently, and proving the two agree pixel-for-pixel over
//     an image deliberately exercising every chunk type -- repeats long enough to trigger
//     QOI_OP_RUN's 62-pixel cap, a colour reused for QOI_OP_INDEX, small deltas for QOI_OP_DIFF,
//     a wider delta for QOI_OP_LUMA, and an alpha change for QOI_OP_RGBA -- is the closest this
//     module gets to a from-scratch conformance test without a second, external QOI tool). That
//     decoder is TEST-ONLY, not shipped: this library offers QOI ENCODE only (see
//     `docs/capabilities.md`'s declared ceiling), decode is explicitly out of scope until a real
//     consumer need for reading QOI back in is measured.
// PT: IMG-ENCODE -- um encoder QOI (Quite OK Image, qoiformat.org) clean-room, escrito só a
//     partir da ESPECIFICAÇÃO PÚBLICA (o formato QOI é deliberadamente um formato de poucas
//     centenas de linhas desenhado pra ser re-derivado da própria especificação, não fruto de
//     engenharia reversa sobre um decoder de referência) -- nenhum `qoi.h` (a implementação de
//     referência de domínio público) foi lido, copiado, ou consultado ao escrever este arquivo,
//     seguindo a disciplina clean-room deste repositório (CLAUDE.md: "ao reimplementar/RE de
//     libs externas, não copiar código -- reimplementar clean-room"). Este é o ÚNICO formato do
//     conjunto IMG-ENCODE que precisou de implementação de fato: os outros seis (PNG/JPG/BMP/
//     TGA/HDR via o stb_image_write vendorizado, PPM escrito à mão em image_encode.cpp) todos já
//     tinham um encoder existente pra chamar. O QOI foi escolhido pra internalização
//     especificamente por ser pequeno, estável, e publicamente documentado -- cabe muito melhor
//     na trajetória de internalização clean-room de longo prazo deste repositório (ver a seção
//     Camada 0 do CLAUDE.md) do que o deflate/zlib do PNG caberia.
//
//     PURO, TOTAL, HEADER-ONLY (mesma disciplina de "costura pura" do `image_decode.hpp`): sem
//     I/O, sem alocação além do próprio crescimento do vector `out` fornecido pelo chamador, sem
//     log (este header não pode alcançar `glintfx::log()` e não precisa -- ver o próprio
//     comentário de `image_encode.cpp` pra onde vive a divisão guarda/log). `qoi_encode_rgba()`
//     NUNCA falha: dado `width > 0`, `height > 0`, e `pixels` apontando pra `width * height * 4`
//     bytes válidos (RGBA8 straight, não-premultiplicado -- a MESMA convenção que
//     `DecodedImagePixels`/`encode_image_memory()` já usam, ver `glintfx/include/glintfx/
//     image.hpp`), ela sempre produz um fluxo de bytes QOI bem-formado. Guardar `width`/
//     `height`/`pixels` em si (a superfície de input hostil: ponteiro nulo, dimensão zero/
//     negativa, overflow de dimensão) é trabalho do CHAMADOR (`image_encode.cpp`), a MESMA
//     divisão de trabalho que o próprio `premultiply_rgba_inplace()`/`compute_content_bbox()` de
//     `image_decode.hpp` já estabelecem entre um helper puro total e o próprio chamador guardado.
//
//     LAYOUT DO FORMATO (qoiformat.org, "The QOI File Format Specification"): um cabeçalho de 14
//     bytes (magic "qoif", width/height como 32 bits BIG-ENDIAN, 1 byte de contagem de canal, 1
//     byte de colorspace -- os DOIS últimos campos são puramente INFORMATIVOS, um decoder não é
//     obrigado a agir sobre eles, então este encoder sempre escreve 4/0 independente de o canal
//     alpha de origem chegar a ser não-255 alguma vez), um fluxo de chunks de diferença-de-pixel
//     de tamanho variável, e um marcador de fim de 8 bytes (sete bytes 0x00 seguidos de um byte
//     0x01). O encoder abaixo é uma implementação literal, integral, dos seis tipos de chunk que
//     a spec define, escolhidos gulosamente na própria ordem de prioridade da spec (uma
//     repetição byte-idêntica colapsa em QOI_OP_RUN; senão um cache rolante de cor de 64 entradas
//     é checado (QOI_OP_INDEX); senão, se o alpha ficou inalterado do pixel anterior, um delta
//     pequeno de RGB é tentado como QOI_OP_DIFF, depois um delta mais largo enviesado por luma
//     como QOI_OP_LUMA; falhando todos esses, o pixel completo é escrito ao pé da letra como
//     QOI_OP_RGB (alpha inalterado) ou QOI_OP_RGBA (alpha mudou)) -- ver os comentários inline em
//     cada ramo abaixo pro layout de bit exato, citado contra os próprios valores de tag da spec.
//
//     POR QUE UM DECODER FEITO À MÃO SÓ PRA TESTE, NÃO EMBARCADO, VERIFICA ISTO (ver o próprio
//     `tests/image_encode_sanity.cpp` deste repo): o oráculo de round-trip -- decodificar o que
//     este encoder acabou de produzir e comparar todo pixel contra a origem -- é uma prova de
//     correção MUITO mais forte que verificar à mão um punhado de bytes codificados (nota do
//     próprio autor deste arquivo: escrever o decoder a partir da MESMA spec, de forma
//     independente, e provar que os dois concordam pixel-a-pixel sobre uma imagem que exercita
//     de propósito todo tipo de chunk -- repetições longas o bastante pra disparar o teto de 62
//     pixels do QOI_OP_RUN, uma cor reusada pra QOI_OP_INDEX, deltas pequenos pra QOI_OP_DIFF, um
//     delta mais largo pra QOI_OP_LUMA, e uma mudança de alpha pra QOI_OP_RGBA -- é o mais perto
//     que este módulo chega de um teste de conformidade do zero sem uma segunda ferramenta QOI
//     externa). Aquele decoder é SÓ-DE-TESTE, não embarcado: esta biblioteca oferece só ENCODE de
//     QOI (ver o teto declarado em `docs/capabilities.md`), decode fica explicitamente fora de
//     escopo até que uma necessidade real de consumidor de ler QOI de volta seja medida.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace glintfx {

// EN: Appends a well-formed QOI byte stream encoding `width * height` RGBA8 straight-alpha
//     pixels to `out` (grown via `push_back`/`insert`, never truncated or cleared first -- the
//     caller decides whether `out` starts empty). See this file's own top comment for the full
//     guard contract (caller-guarded, this function is unconditionally total over any
//     `width > 0, height > 0, pixels != nullptr` input) and format layout.
// PT: Acrescenta um fluxo de bytes QOI bem-formado codificando `width * height` pixels RGBA8
//     alpha straight em `out` (crescido via `push_back`/`insert`, nunca truncado ou limpo antes
//     -- o chamador decide se `out` começa vazio). Ver o próprio comentário de topo deste arquivo
//     pro contrato de guarda completo (guardado pelo chamador, esta função é incondicionalmente
//     total sobre qualquer input `width > 0, height > 0, pixels != nullptr`) e o layout do
//     formato.
inline void qoi_encode_rgba(const unsigned char* pixels, int width, int height,
                            std::vector<unsigned char>& out) {
  const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

  // EN: Rough upper-bound reserve (worst case is QOI_OP_RGBA's 5 bytes/pixel; a real image never
  //     hits that on every single pixel) -- pure amortization, correctness does not depend on it
  //     (std::vector grows on its own if the reserve undershoots).
  // PT: Reserva de limite superior aproximado (o pior caso é os 5 bytes/pixel do QOI_OP_RGBA; uma
  //     imagem real nunca bate nisso em todo pixel) -- pura amortização, a correção não depende
  //     disto (std::vector cresce sozinho se a reserva ficar aquém).
  out.reserve(out.size() + 14 + pixel_count * 5 + 8);

  auto push_u32be = [&out](std::uint32_t v) {
    out.push_back(static_cast<unsigned char>((v >> 24) & 0xFFu));
    out.push_back(static_cast<unsigned char>((v >> 16) & 0xFFu));
    out.push_back(static_cast<unsigned char>((v >> 8) & 0xFFu));
    out.push_back(static_cast<unsigned char>(v & 0xFFu));
  };

  // EN: Header -- magic, dimensions (big-endian per spec), channels=4/colorspace=0 (both
  //     informative-only, see this file's top comment).
  // PT: Cabeçalho -- magic, dimensões (big-endian conforme a spec), channels=4/colorspace=0
  //     (ambos só-informativos, ver o comentário de topo deste arquivo).
  out.push_back('q');
  out.push_back('o');
  out.push_back('i');
  out.push_back('f');
  push_u32be(static_cast<std::uint32_t>(width));
  push_u32be(static_cast<std::uint32_t>(height));
  out.push_back(4); // channels (informative)
  out.push_back(0); // colorspace (informative)

  struct Px {
    unsigned char r = 0, g = 0, b = 0, a = 0;
    bool operator==(const Px& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
  };

  // EN: 64-entry rolling colour cache, spec-mandated initial state: every slot zeroed
  //     (RGBA(0,0,0,0)) -- a decoder starts with the identical initial state, so an encoder/
  //     decoder pair agree without any out-of-band synchronization.
  // PT: Cache rolante de cor de 64 entradas, estado inicial mandado pela spec: todo slot zerado
  //     (RGBA(0,0,0,0)) -- um decoder começa com o estado inicial idêntico, então um par
  //     encoder/decoder concorda sem sincronização fora-de-banda nenhuma.
  Px index[64];

  // EN: Spec-mandated initial "previous pixel": opaque black, NOT RGBA(0,0,0,0) -- this matters
  //     for the very first pixel's own diff/luma computation.
  // PT: "Pixel anterior" inicial mandado pela spec: preto opaco, NÃO RGBA(0,0,0,0) -- isto
  //     importa pro próprio cálculo de diff/luma do primeiro pixel.
  Px prev{0, 0, 0, 255};

  int run = 0;

  // EN: Wraps a signed delta (range up to ±255 for two arbitrary byte values) into the signed
  //     8-bit value an unsigned-byte SUBTRACTION-then-reinterpretation would produce -- the
  //     conversion int -> unsigned char is a well-defined modulo-256 reduction for ANY int per
  //     the C++ standard, and the subsequent unsigned char -> signed char conversion is a
  //     well-defined two's-complement reinterpretation as of C++20 (this library's own C++23
  //     target, per CLAUDE.md's stack table, is well past that guarantee; every real-world
  //     compiler -- clang/gcc/MSVC -- already behaves this way pre-C++20 too). This is the exact
  //     arithmetic the QOI spec's own "wrapping" diff computation requires (a diff of, e.g.,
  //     prev=255, cur=0 must read as +1, not -255).
  // PT: Envolve um delta assinado (faixa de até ±255 pra dois valores de byte arbitrários) no
  //     valor assinado de 8 bits que uma SUBTRAÇÃO de byte sem sinal seguida de reinterpretação
  //     produziria -- a conversão int -> unsigned char é uma redução módulo-256 bem definida pra
  //     QUALQUER int pelo padrão C++, e a conversão subsequente unsigned char -> signed char é
  //     uma reinterpretação de complemento-de-dois bem definida a partir do C++20 (o próprio alvo
  //     C++23 desta biblioteca, pela tabela de stack do CLAUDE.md, está bem além dessa garantia;
  //     todo compilador do mundo real -- clang/gcc/MSVC -- já se comporta assim mesmo antes do
  //     C++20). É exatamente a aritmética que o próprio cálculo de diff "com wraparound" da spec
  //     QOI exige (um diff de, ex., prev=255, cur=0 precisa ler como +1, não -255).
  auto wrap8 = [](int d) -> int {
    return static_cast<int>(static_cast<signed char>(static_cast<unsigned char>(d)));
  };

  for (std::size_t i = 0; i < pixel_count; ++i) {
    const Px cur{pixels[i * 4 + 0], pixels[i * 4 + 1], pixels[i * 4 + 2], pixels[i * 4 + 3]};
    const bool is_last = (i == pixel_count - 1);

    if (cur == prev) {
      ++run;
      // EN: QOI_OP_RUN's own 6-bit field caps a single run chunk at 62 pixels (run-1 must fit
      //     0..61 -- 62/63 are reserved, they collide with the QOI_OP_RGB/RGBA tag bytes 0xFE/
      //     0xFF, see the tag-byte comment below) -- flush at exactly 62, or at the very last
      //     pixel of the image (an unflushed run at end-of-image would be silently dropped).
      // PT: O próprio campo de 6 bits do QOI_OP_RUN limita um único chunk de run a 62 pixels
      //     (run-1 precisa caber em 0..61 -- 62/63 são reservados, colidem com os bytes de tag
      //     QOI_OP_RGB/RGBA 0xFE/0xFF, ver o comentário de byte-de-tag abaixo) -- descarrega
      //     exatamente em 62, ou no último pixel da imagem (uma run não descarregada no fim da
      //     imagem seria silenciosamente perdida).
      if (run == 62 || is_last) {
        out.push_back(static_cast<unsigned char>(0xC0u | static_cast<unsigned>(run - 1)));
        run = 0;
      }
      continue;
    }

    // EN: `cur != prev` -- flush any in-flight run FIRST (a run only ever contains copies of
    //     `prev`; the moment a differing pixel arrives, the run is over).
    // PT: `cur != prev` -- descarrega qualquer run em andamento PRIMEIRO (uma run só contém
    //     cópias de `prev`; no instante em que um pixel diferente chega, a run acabou).
    if (run > 0) {
      out.push_back(static_cast<unsigned char>(0xC0u | static_cast<unsigned>(run - 1)));
      run = 0;
    }

    // EN: QOI_OP_INDEX -- spec hash: (r*3 + g*5 + b*7 + a*11) % 64. `& 63` == `% 64` for a
    //     power-of-two modulus, and the tag byte for QOI_OP_INDEX is `0b00xxxxxx` -- the raw
    //     0..63 hash value already has both high bits clear, so it IS the tag byte, no OR needed.
    // PT: QOI_OP_INDEX -- hash da spec: (r*3 + g*5 + b*7 + a*11) % 64. `& 63` == `% 64` pra um
    //     módulo potência-de-dois, e o byte de tag do QOI_OP_INDEX é `0b00xxxxxx` -- o valor cru
    //     de hash 0..63 já tem os dois bits altos zerados, então ELE JÁ É o byte de tag, sem OR
    //     necessário.
    const unsigned hash = (static_cast<unsigned>(cur.r) * 3u + static_cast<unsigned>(cur.g) * 5u +
                           static_cast<unsigned>(cur.b) * 7u + static_cast<unsigned>(cur.a) * 11u) &
                          63u;

    if (index[hash] == cur) {
      out.push_back(static_cast<unsigned char>(hash));
      prev = cur;
      continue;
    }
    index[hash] = cur;

    if (cur.a == prev.a) {
      const int dr = wrap8(static_cast<int>(cur.r) - static_cast<int>(prev.r));
      const int dg = wrap8(static_cast<int>(cur.g) - static_cast<int>(prev.g));
      const int db = wrap8(static_cast<int>(cur.b) - static_cast<int>(prev.b));

      if (dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1) {
        // EN: QOI_OP_DIFF -- tag `0b01rrggbb`, each 2-bit field biased by +2 (stored 0..3 <->
        //     actual diff -2..1).
        // PT: QOI_OP_DIFF -- tag `0b01rrggbb`, cada campo de 2 bits enviesado por +2 (armazenado
        //     0..3 <-> diff real -2..1).
        out.push_back(static_cast<unsigned char>(0x40u | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2)));
      } else {
        const int dr_dg = dr - dg;
        const int db_dg = db - dg;
        if (dg >= -32 && dg <= 31 && dr_dg >= -8 && dr_dg <= 7 && db_dg >= -8 && db_dg <= 7) {
          // EN: QOI_OP_LUMA -- 2 bytes. Byte 1: tag `0b10gggggg`, green diff biased by +32
          //     (stored 0..63 <-> actual -32..31). Byte 2: `0b rrrr bbbb`, (dr-dg) and (db-dg)
          //     each biased by +8 (stored 0..15 <-> actual -8..7).
          // PT: QOI_OP_LUMA -- 2 bytes. Byte 1: tag `0b10gggggg`, diff de verde enviesado por +32
          //     (armazenado 0..63 <-> real -32..31). Byte 2: `0b rrrr bbbb`, (dr-dg) e (db-dg)
          //     cada um enviesado por +8 (armazenado 0..15 <-> real -8..7).
          out.push_back(static_cast<unsigned char>(0x80u | (dg + 32)));
          out.push_back(static_cast<unsigned char>(((dr_dg + 8) << 4) | (db_dg + 8)));
        } else {
          // EN: QOI_OP_RGB -- tag byte 0xFE, then raw R/G/B (alpha implicitly unchanged from
          //     `prev`).
          // PT: QOI_OP_RGB -- byte de tag 0xFE, depois R/G/B crus (alpha implicitamente
          //     inalterado de `prev`).
          out.push_back(0xFE);
          out.push_back(cur.r);
          out.push_back(cur.g);
          out.push_back(cur.b);
        }
      }
    } else {
      // EN: QOI_OP_RGBA -- tag byte 0xFF, then raw R/G/B/A (alpha itself changed from `prev`, so
      //     it must be written explicitly -- none of DIFF/LUMA/RGB can express an alpha change).
      // PT: QOI_OP_RGBA -- byte de tag 0xFF, depois R/G/B/A crus (o próprio alpha mudou de
      //     `prev`, então precisa ser escrito explicitamente -- nenhum de DIFF/LUMA/RGB consegue
      //     expressar uma mudança de alpha).
      out.push_back(0xFF);
      out.push_back(cur.r);
      out.push_back(cur.g);
      out.push_back(cur.b);
      out.push_back(cur.a);
    }

    prev = cur;
  }

  // EN: End-of-stream marker, spec-fixed: seven 0x00 bytes then one 0x01 byte.
  // PT: Marcador de fim-de-fluxo, fixo pela spec: sete bytes 0x00 depois um byte 0x01.
  for (int k = 0; k < 7; ++k)
    out.push_back(0);
  out.push_back(1);
}

} // namespace glintfx
