// SPDX-License-Identifier: Apache-2.0
// EN: DEC-FORMAT-SURFACE (W22, 2026-07-30) -- consumer-driven, security-flavoured: pure unit
//     test for `glintfx::decode_png_file()` (`glintfx/image.hpp`), the PNG-ONLY sibling of
//     `decode_image_file()` added by this slice. No RmlUi, no GL, no window -- same "no
//     display to isolate here" reasoning as image_sanity.cpp/image_decode_sanity.cpp.
//     Registered unconditionally -- runs in BOTH build configurations.
//
//     THE REGRESSION THIS FILE EXISTS TO PIN: GusWorld measured that `decode_image_file()`'s
//     general PNG/JPG/TGA dispatch (`STBI_ONLY_TGA`, `src/stb_image_impl.cpp`) accepts an
//     82-byte buffer -- an 18-byte plausible-looking TGA header (image_type=2, 4096x4096,
//     32bpp) followed by 64 bytes of unrelated noise -- and decodes it "successfully" into a
//     4096x4096, all-zero-alpha RGBA8 image (67 108 864 bytes from 82 bytes of input). Group 1
//     below reproduces that EXACT finding (the coador itself, on `decode_image_file()`, still
//     unrestricted -- unchanged behaviour, not this slice's job to alter) and then proves
//     `decode_png_file()` closes it for a caller who opts in.
//
//     `kFakeTgaGarbage82` below is BYTE-FOR-BYTE the `fake_tga_garbage.png` fixture the
//     consumer's own deterministic corpus generator produces (`gen_corpus.py`, GusWorld repo
//     `assets/probe-decode/`, fixed seed `20260730` for the trailing 64 noise bytes) --
//     verified by regenerating the corpus and comparing (`sha256sum`:
//     `1dc442ea6d179c0e9f2d13f6ce1a4d68d9ff8cb7afacc514b8851a7cb93a5982`, 82 bytes). Embedded
//     as a literal array here (not committed as a binary fixture file) for the same reason
//     this file's own hostile-corpus siblings (image_decode_sanity.cpp/image_sanity.cpp) embed
//     PNG-shape byte arrays inline rather than adding new tracked binaries, AND because using
//     the consumer's OWN bytes (not a byte pattern re-derived from the bug's prose
//     description) is the whole point: "if the test of the fix uses the SAME input that caught
//     the bug, it proves the fix catches THIS case; a test written from the description risks
//     exercising a neighbour and passing for the wrong reason" (DEC-PROBE-CORPUS, TODO.md).
//
//     Groups 3-5 exercise the signature check with minimal, deliberately-NOT-real-image byte
//     sequences (a real encoder is not needed: in NORMAL operation `decode_png_file()` rejects
//     all of these at the first-8-bytes check, before any decoder ever runs) -- JPEG magic, BMP
//     magic, plain text, a too-short buffer, and a buffer whose first 8 bytes ARE the PNG
//     signature but whose body is garbage. PRECISION, stated up front because Group 3's own
//     comment restates it in more depth: 3a/3b/3c are OBSERVABLE-BEHAVIOUR checks, not a
//     mutation-proof of the gate -- with `has_png_signature()` disarmed, all three stay green,
//     because the underlying PNG/JPG/TGA decode independently rejects the same malformed stubs
//     (redundant protection, confirmed by execution). 1b (Group 1 above) is the ONLY assertion
//     in this file that flips red under that exact mutation, and is therefore the only one that
//     actually PINS the signature gate's own existence; Group 5's garbage-after-a-valid-
//     signature case proves a DIFFERENT, still-true fact (the gate is a FIRST check, not a
//     substitute for the real PNG decode that still runs afterwards) and does not depend on this
//     distinction. Group 6 is the full file-I/O hostile corpus (null path, nonexistent path,
//     empty path) mirrored from image_sanity.cpp's own Group 4.
// PT: DEC-FORMAT-SURFACE (W22, 2026-07-30) -- consumer-driven, com tempero de segurança: teste
//     unit puro para `glintfx::decode_png_file()` (`glintfx/image.hpp`), a irmã SÓ-PNG do
//     `decode_image_file()` somada por esta fatia. Sem RmlUi, sem GL, sem janela -- mesma
//     racional "nada relacionado a display pra isolar aqui" de image_sanity.cpp/
//     image_decode_sanity.cpp. Registrado incondicionalmente -- roda nas DUAS configurações de
//     build.
//
//     A REGRESSÃO QUE ESTE ARQUIVO EXISTE PRA FIXAR: o GusWorld mediu que o dispatch geral
//     PNG/JPG/TGA do `decode_image_file()` (`STBI_ONLY_TGA`, `src/stb_image_impl.cpp`) aceita
//     um buffer de 82 bytes -- um header TGA plausível de 18 bytes (image_type=2, 4096x4096,
//     32bpp) seguido de 64 bytes de ruído sem relação -- e decodifica "com sucesso" numa
//     imagem RGBA8 4096x4096 alpha-zero-em-tudo (67.108.864 bytes a partir de 82 bytes de
//     input). O Grupo 1 abaixo reproduz esse achado EXATO (o próprio coador, sobre o
//     `decode_image_file()`, ainda irrestrito -- comportamento inalterado, não é trabalho
//     desta fatia mudar isso) e depois prova que o `decode_png_file()` fecha isso pra um
//     chamador que opta por ele.
//
//     `kFakeTgaGarbage82` abaixo é BYTE-A-BYTE a própria fixture `fake_tga_garbage.png` que o
//     gerador de corpus determinístico do consumidor produz (`gen_corpus.py`, repo GusWorld
//     `assets/probe-decode/`, semente fixa `20260730` pros 64 bytes de ruído finais) --
//     verificado regenerando o corpus e comparando (`sha256sum`:
//     `1dc442ea6d179c0e9f2d13f6ce1a4d68d9ff8cb7afacc514b8851a7cb93a5982`, 82 bytes). Embutido
//     como array literal aqui (não commitado como arquivo binário de fixture) pelo mesmo
//     motivo dos próprios irmãos de corpus hostil deste projeto (image_decode_sanity.cpp/
//     image_sanity.cpp) embutirem arrays de bytes em forma de PNG inline em vez de somar
//     binários novos rastreados, E porque usar os PRÓPRIOS bytes do consumidor (não um padrão
//     de bytes re-derivado da descrição em prosa do bug) é o ponto inteiro: "se o teste do
//     conserto usar a MESMA entrada que pegou o bug, ele prova que o conserto pega ESTE caso;
//     um teste escrito da descrição corre o risco de exercitar um vizinho e passar por motivo
//     errado" (DEC-PROBE-CORPUS, TODO.md).
//
//     Os Grupos 3-5 exercitam a checagem de assinatura com sequências de bytes mínimas,
//     deliberadamente NÃO-imagem-real (um encoder de verdade não é necessário: em operação
//     NORMAL o `decode_png_file()` rejeita todas elas na checagem dos primeiros 8 bytes, antes
//     de qualquer decoder rodar) -- magic de JPEG, magic de BMP, texto puro, um buffer curto
//     demais, e um buffer cujos primeiros 8 bytes SÃO a assinatura PNG mas cujo corpo é lixo.
//     PRECISÃO, dita de antemão porque o próprio comentário do Grupo 3 reafirma isto com mais
//     profundidade: 3a/3b/3c são checagens de COMPORTAMENTO OBSERVÁVEL, não uma prova-por-
//     mutação da guarda -- com o `has_png_signature()` desarmado, as três continuam verdes,
//     porque o próprio decode PNG/JPG/TGA rejeita independentemente os mesmos stubs malformados
//     (proteção redundante, confirmada por execução). O 1b (Grupo 1 acima) é a ÚNICA asserção
//     deste arquivo que vira vermelha sob essa mutação exata, e é portanto a única que de fato
//     PINA a própria existência da guarda de assinatura; o caso de lixo-após-assinatura-válida
//     do Grupo 5 prova um fato DIFERENTE, ainda verdadeiro (a guarda é uma PRIMEIRA checagem,
//     não um substituto do decode PNG de verdade que ainda roda depois) e não depende desta
//     distinção. O Grupo 6 é o corpus hostil completo de I/O de arquivo (path nulo, path
//     inexistente, path vazio) espelhado do próprio Grupo 4 de image_sanity.cpp.
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
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
// EN: kFakeTgaGarbage82 -- byte-for-byte the consumer's own fake_tga_garbage.png fixture. See
//     this file's own top comment for provenance (gen_corpus.py, fixed seed 20260730,
//     sha256 verified).
// PT: kFakeTgaGarbage82 -- byte-a-byte a própria fixture fake_tga_garbage.png do consumidor.
//     Ver o comentário de topo deste arquivo pra proveniência (gen_corpus.py, semente fixa
//     20260730, sha256 verificado).
// ---------------------------------------------------------------------------
const unsigned char kFakeTgaGarbage82[82] = {
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x10, 0x20, 0x00, 0x59, 0x82, 0x56, 0x11, 0xcc, 0x1e,
    0x58, 0xa8, 0xef, 0xb6, 0x24, 0x7e, 0x97, 0xb8, 0x64, 0x73, 0xce, 0x8f,
    0x7d, 0x23, 0x21, 0xf4, 0xc3, 0xc9, 0x75, 0xd5, 0xca, 0xf8, 0xe3, 0x16,
    0xb1, 0x32, 0x3f, 0x40, 0x8a, 0x7d, 0x84, 0xb4, 0xfa, 0x3e, 0x5e, 0x8b,
    0x01, 0xb5, 0x24, 0xf8, 0xbd, 0xdd, 0x2a, 0xb5, 0xa0, 0xeb, 0xc5, 0xaa,
    0x72, 0xcc, 0x16, 0x88, 0x8f, 0xc9, 0x67, 0xcb, 0xcc, 0x73};

// EN: Writes `bytes` to a fresh file at `path` (overwriting), for the file-entry-point groups
//     below. Returns true on success -- callers `check()` it.
// PT: Escreve `bytes` num arquivo novo em `path` (sobrescrevendo), pros grupos de ponto de
//     entrada de arquivo abaixo. Devolve true em sucesso -- os chamadores fazem `check()`.
bool write_file(const char* path, const unsigned char* bytes, std::size_t len) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f)
    return false;
  f.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(len));
  return static_cast<bool>(f);
}

} // namespace

using namespace glintfx;

int main() {
  // ===========================================================================================
  // EN: Group 1 -- the regression itself. 1a reproduces the coador on decode_image_file() (the
  //     GENERAL dispatch, unrestricted, unchanged by this slice): the 82-byte fake TGA header
  //     decodes "successfully" into a 4096x4096 all-zero-alpha image. 1b proves
  //     decode_png_file() rejects the EXACT SAME bytes.
  // PT: Grupo 1 -- a própria regressão. 1a reproduz o coador sobre decode_image_file() (o
  //     dispatch GERAL, irrestrito, inalterado por esta fatia): o header TGA falso de 82 bytes
  //     decodifica "com sucesso" numa imagem 4096x4096 alpha-zero-em-tudo. 1b prova que o
  //     decode_png_file() rejeita os MESMOS bytes exatos.
  // ===========================================================================================
  {
    const DecodedImagePixels general = decode_image_memory(kFakeTgaGarbage82, sizeof(kFakeTgaGarbage82));
    check(general.ok,
          "1a: decode_image_memory (general dispatch) STILL accepts the 82-byte fake "
          "TGA header -- documents the coador this slice does not change on the "
          "general entry point");
    check(general.width == 4096 && general.height == 4096,
          "1a: general dispatch decodes it as 4096x4096 (the measured coador shape)");
    check(general.pixels.size() == 4096u * 4096u * 4u,
          "1a: general dispatch allocates the full 67 108 864-byte all-zero-alpha buffer");
  }
  {
    check(write_file("dec_format_surface_fake_tga.png", kFakeTgaGarbage82, sizeof(kFakeTgaGarbage82)),
          "1b setup: fake TGA garbage written to disk for the file-entry-point call");
    const DecodedImagePixels restricted = decode_png_file("dec_format_surface_fake_tga.png");
    check(!restricted.ok,
          "1b: decode_png_file REJECTS the exact 82-byte fake-TGA-header bytes that "
          "decode_image_file/decode_image_memory accept (ok == false) -- THE regression this "
          "file exists to pin");
  }

  // ===========================================================================================
  // EN: Group 2 -- valid PNG is still accepted. Reuses the SAME image_decode_rgba_2x2.png
  //     fixture image_decode_sanity.cpp/image_sanity.cpp already establish in the shared build
  //     dir (no second configure_file() call needed).
  // PT: Grupo 2 -- PNG válido continua sendo aceito. Reusa a MESMA fixture
  //     image_decode_rgba_2x2.png que image_decode_sanity.cpp/image_sanity.cpp já estabelecem
  //     no build dir compartilhado (nenhuma segunda chamada configure_file() necessária).
  // ===========================================================================================
  {
    const DecodedImagePixels decoded = decode_png_file("image_decode_rgba_2x2.png");
    check(decoded.ok, "2: decode_png_file accepts a real PNG (ok == true)");
    check(decoded.width == 2 && decoded.height == 2, "2: real PNG decodes to the expected 2x2 dims");
    check(decoded.pixels.size() == 16, "2: real PNG pixel buffer is width*height*4 == 16 bytes");
  }

  // ===========================================================================================
  // EN: Group 3 -- OBSERVABLE-BEHAVIOUR checks, NOT a mutation-proof of the signature gate: for
  //     none of 3a/3b/3c does removing/disarming has_png_signature() flip the result to
  //     ok == true. JPEG-magic and BMP-magic bytes here are 16-byte STUBS (magic + zeros), never
  //     a complete, decodable image -- the underlying PNG/JPG/TGA decode
  //     (decode_image_memory()) already rejects all three independently of the signature gate
  //     (no valid JPEG/BMP/TGA structure follows the magic, and plain text matches no format's
  //     magic at all). Confirmed by mutation (2026-07-30, decode_png_file()'s own DEC-FORMAT-
  //     SURFACE mutation run): with has_png_signature() disarmed, 1b below is the ONLY assertion
  //     in this file that flips red -- 3a/3b/3c stay green because rejection happens downstream,
  //     at decode, not at the gate this Group is nominally about. They are kept here anyway
  //     because they exercise a REAL, separately-valuable contract this file's own top comment
  //     already states (rejection is by content, decode_png_file() needs no real encoder to
  //     prove it, extension is never consulted) -- just not the ONE fact their old wording
  //     implied. 1b (fake TGA garbage, Group 1 above) is the ONLY assertion in this file that
  //     pins the signature gate itself; do not read 3a/3b/3c as doing that job too.
  // PT: Grupo 3 -- checagens de COMPORTAMENTO OBSERVÁVEL, NÃO uma prova-por-mutação da guarda de
  //     assinatura: em nenhum de 3a/3b/3c remover/desarmar o has_png_signature() vira o
  //     resultado pra ok == true. Os bytes de magic JPEG/BMP aqui são STUBS de 16 bytes (magic +
  //     zeros), nunca uma imagem completa, decodificável -- o próprio decode PNG/JPG/TGA
  //     (decode_image_memory()) já rejeita os três independentemente da guarda de assinatura
  //     (nenhuma estrutura JPEG/BMP/TGA válida segue o magic, e texto puro não casa magic de
  //     formato nenhum). Confirmado por mutação (2026-07-30, na própria rodada de mutação
  //     DEC-FORMAT-SURFACE do decode_png_file()): com has_png_signature() desarmado, o 1b
  //     abaixo é a ÚNICA asserção deste arquivo que vira vermelha -- 3a/3b/3c continuam verdes
  //     porque a rejeição acontece rio abaixo, no decode, não na guarda que este Grupo
  //     nominalmente diz respeitar. Ficam aqui mesmo assim porque exercitam um contrato REAL,
  //     valioso à parte, que o próprio comentário de topo deste arquivo já declara (rejeição é
  //     por conteúdo, decode_png_file() não precisa de encoder de verdade pra provar isso,
  //     extensão nunca é consultada) -- só não o ÚNICO fato que a redação antiga implicava. O 1b
  //     (lixo de TGA falso, Grupo 1 acima) é a ÚNICA asserção deste arquivo que pina a própria
  //     guarda de assinatura; não leia 3a/3b/3c como fazendo esse trabalho também.
  // ===========================================================================================
  {
    const unsigned char jpeg_like[16] = {0xFF, 0xD8, 0xFF, 0xE0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    check(write_file("dec_format_surface_jpeg_magic.png", jpeg_like, sizeof(jpeg_like)),
          "3a setup: JPEG-magic bytes written");
    check(!decode_png_file("dec_format_surface_jpeg_magic.png").ok,
          "3a: decode_png_file rejects JPEG-magic bytes even with a .png-suffixed path "
          "(rejection is by content -- both the signature gate AND the underlying decode "
          "independently reject this malformed stub, never by extension)");
  }
  {
    const unsigned char bmp_like[16] = {'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    check(write_file("dec_format_surface_bmp_magic.png", bmp_like, sizeof(bmp_like)),
          "3b setup: BMP-magic bytes written");
    check(!decode_png_file("dec_format_surface_bmp_magic.png").ok,
          "3b: decode_png_file rejects BMP-magic bytes");
  }
  {
    const char* text = "isto nao e uma imagem, e texto puro\n";
    check(write_file("dec_format_surface_text.png",
                     reinterpret_cast<const unsigned char*>(text), std::strlen(text)),
          "3c setup: plain text written");
    check(!decode_png_file("dec_format_surface_text.png").ok,
          "3c: decode_png_file rejects plain text renamed to .png");
  }

  // ===========================================================================================
  // EN: Group 4 -- boundary of the signature check itself: a buffer shorter than the 8-byte
  //     PNG signature, and the empty-file case (already covered generically, re-asserted here
  //     at decode_png_file's own boundary).
  // PT: Grupo 4 -- fronteira da própria checagem de assinatura: um buffer mais curto que a
  //     assinatura PNG de 8 bytes, e o caso de arquivo vazio (já coberto genericamente,
  //     reafirmado aqui na própria fronteira de decode_png_file).
  // ===========================================================================================
  {
    const unsigned char short_buf[5] = {0x89, 'P', 'N', 'G', '\r'}; // only 5 of the 8 sig bytes.
    check(write_file("dec_format_surface_short.png", short_buf, sizeof(short_buf)),
          "4a setup: 5-byte (short) PNG-signature-prefix file written");
    check(!decode_png_file("dec_format_surface_short.png").ok,
          "4a: decode_png_file rejects a file shorter than the 8-byte PNG signature "
          "(no out-of-bounds read attempted -- has_png_signature's own length guard)");
  }
  {
    const unsigned char empty[1] = {0};
    check(write_file("dec_format_surface_empty.png", empty, 0),
          "4b setup: 0-byte file written");
    check(!decode_png_file("dec_format_surface_empty.png").ok,
          "4b: decode_png_file rejects a 0-byte file (same pre-allocation size guard as "
          "decode_image_file)");
  }

  // ===========================================================================================
  // EN: Group 5 -- the signature check is a FIRST gate, not a substitute for the real PNG
  //     decode: a buffer whose first 8 bytes ARE the exact PNG signature but whose body is
  //     garbage (no valid IHDR at all) still fails -- at decode, not at the signature check.
  // PT: Grupo 5 -- a checagem de assinatura é uma PRIMEIRA guarda, não um substituto do decode
  //     PNG de verdade: um buffer cujos primeiros 8 bytes SÃO a assinatura PNG exata mas cujo
  //     corpo é lixo (nenhum IHDR válido) ainda falha -- no decode, não na checagem de
  //     assinatura.
  // ===========================================================================================
  {
    unsigned char sig_then_garbage[24] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    for (int i = 8; i < 24; ++i)
      sig_then_garbage[i] = static_cast<unsigned char>((i * 37 + 11) & 0xFF);
    check(write_file("dec_format_surface_sig_garbage.png", sig_then_garbage, sizeof(sig_then_garbage)),
          "5 setup: valid-signature-then-garbage file written");
    check(!decode_png_file("dec_format_surface_sig_garbage.png").ok,
          "5: decode_png_file passes the signature gate but still fails the real PNG decode "
          "for a garbage body (proves the signature check is not a decode substitute)");
  }

  // ===========================================================================================
  // EN: Group 6 -- file-I/O hostile corpus, mirrored from image_sanity.cpp's own Group 4. Also:
  //     a real decode_png_file() call still works right after the corpus (same "still usable"
  //     oracle idiom that file's own Group 4 uses).
  // PT: Grupo 6 -- corpus hostil de I/O de arquivo, espelhado do próprio Grupo 4 de
  //     image_sanity.cpp. Também: uma chamada real decode_png_file() ainda funciona logo após
  //     o corpus (mesmo idioma-oráculo "ainda usável" que o próprio Grupo 4 daquele arquivo
  //     usa).
  // ===========================================================================================
  {
    check(!decode_png_file(nullptr).ok, "6: nullptr path rejected (ok == false)");
    check(!decode_png_file("this/path/does/not/exist.png").ok, "6: nonexistent path rejected (ok == false)");
    check(!decode_png_file("").ok, "6: empty-string path rejected (ok == false)");

    const DecodedImagePixels real = decode_png_file("image_decode_rgba_2x2.png");
    check(real.ok && real.width == 2 && real.height == 2,
          "6: a real decode_png_file() call still works right after the corpus");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "image_png_only_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("image_png_only_sanity: PASS");
  return 0;
}
