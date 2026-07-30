// SPDX-License-Identifier: MPL-2.0
// EN: DEC-MOVE (A-4) + DEC-NOTHROW (A-1), W21 2026-07-30 -- consumer-driven (GusWorld reported,
//     blocked): the PUBLIC decode pair (`decode_image_file()`/`decode_image_memory()`,
//     `glintfx/image.hpp`) had a `const`-defeated `std::move()` (a silent extra copy of the pixel
//     buffer, `src/image.cpp`) AND zero `try`/`catch` around the two allocations that can
//     genuinely throw `std::bad_alloc` for a REAL input (a paletted PNG or TGA whose decoded
//     RGBA8 size is LARGER than its own intermediate stb buffer -- the consumer's own measured
//     repro: 12000x12000 paletted PNG under `ulimit -v 950000`, `stbi_load` loads, this
//     library's own decode throws). This file proves BOTH fixes with REAL, observable evidence,
//     not a hypothetical:
//       Group A (DEC-MOVE) -- a global `::operator new`/`::operator delete` override, SCOPED by a
//       watch flag + an EXACT byte-size match (not a coarse "did any allocation happen"), counts
//       how many vector-sized allocations of a distinctively-sized (non-power-of-two, collision-
//       unlikely) decoded pixel buffer occur during ONE `decode_image_memory()` call. The fixed
//       code allocates exactly ONCE (the `assign()` inside `decode_straight_rgba()`,
//       `image_decode.hpp` -- irreducible without adopting stb's `malloc`-owned pointer into a
//       `std::vector`, a mismatched-allocator UB trap this library deliberately does not take,
//       see `src/image.cpp`'s own top comment); the `const`-bugged code allocates TWICE (the same
//       `assign()` PLUS `DecodedImagePixels::pixels`'s own copy-assignment). Deterministic, no OS
//       memory limit, no dependency on the host machine's free memory.
//       Group B (DEC-NOTHROW) -- a TWO-STAGE `fork()` + `setrlimit(RLIMIT_AS, ...)` oracle that
//       forces a REAL allocation failure inside the decode path (not a mock, not a hypothetical),
//       on a hand-rolled PALETTED (indexed) PNG matching the consumer's own repro shape (see
//       `build_indexed_png()`'s own top comment for why paletted specifically). STAGE 1/2
//       CALIBRATES: an unconstrained child decodes the SAME fixture to completion and reports
//       back, via a pipe, `VmPeak - baseline` -- the REAL, measured peak virtual-address-space
//       growth the whole call needed. STAGE 2/2 SQUEEZES: a second, constrained child sets
//       `RLIMIT_AS` to `baseline + (measured peak * 3/4)`, comfortably short of what STAGE 1/2
//       proved was needed, so SOME allocation inside the call must genuinely fail. Both children's
//       own baselines are measured from THIS process's OWN `/proc/self/status` `VmSize` (read
//       right after each `fork()`, before any decode-related allocation), so the whole budget is
//       dynamic/self-calibrating and does not depend on the host/CI machine's actual free memory
//       (`feedback_auditoria_domino`'s own "the test attacked the EXAGGERATION, not the BOUNDARY"
//       lesson -- an absolute memory-limit constant would either never bind on a machine with more
//       RAM, or bind too early/late depending on this binary's own baseline VmSize on that host).
//       The squeezed child calls `decode_image_file()` end-to-end (the consumer's own exact call
//       shape: a real file on disk) and the PARENT asserts, via `waitpid()`, that it was NOT
//       killed by a signal (an uncaught exception -> `std::terminate()` -> `abort()` -> `SIGABRT`
//       -- exactly the DEC-NOTHROW regression this test exists to catch) and exited cleanly
//       reporting `ok == false` (a genuine, forced allocation failure correctly degraded, not
//       silently swallowed into a wrong "success"). This two-stage MEASURE-then-SQUEEZE design
//       replaced two earlier, hand-derived FIXED multipliers (`baseline + 1.5*S`, then
//       `baseline + 1.75*S`, `S` the decoded pixel-buffer size) that BOTH undershot the real peak
//       (measured, via mutation testing removing the `try`/`catch`: the true peak is close to
//       2.75*S, not the ~1.5-2*S a hand trace of `stb_image.h`'s own allocation sequence
//       predicted) and squeezed so tight they kept failing INSIDE stb's own `malloc()` -- an
//       ALREADY-safe path that stayed GREEN even with the fix removed, silently testing nothing;
//       see the Group B block's own top comment for that measurement and the full derivation.
//     Linux-only (`/proc/self/status`, `fork()`, `setrlimit()`) -- matches this whole library's
//     own "Linux x86-64" platform scope (`CLAUDE.md`), no `#ifdef` needed. Registered
//     UNCONDITIONALLY (no display/GL/RmlUi dependency whatsoever, same "nothing to isolate here"
//     reasoning as image_sanity.cpp/image_decode_sanity.cpp) -- runs in BOTH build
//     configurations.
// PT: DEC-MOVE (A-4) + DEC-NOTHROW (A-1), W21 2026-07-30 -- consumer-driven (reportado pelo
//     GusWorld, BLOQUEADO): o par PÚBLICO de decode (`decode_image_file()`/
//     `decode_image_memory()`, `glintfx/image.hpp`) tinha um `std::move()` derrotado por `const`
//     (uma cópia extra silenciosa do buffer de pixel, `src/image.cpp`) E zero `try`/`catch` em
//     volta das duas alocações que podem genuinamente lançar `std::bad_alloc` pra um input REAL
//     (um PNG ou TGA paletizado cujo tamanho RGBA8 decodificado é MAIOR que o próprio buffer
//     intermediário do stb -- o próprio repro medido pelo consumidor: PNG paletizado 12000x12000
//     sob `ulimit -v 950000`, `stbi_load` carrega, o decode desta biblioteca lança). Este arquivo
//     prova OS DOIS consertos com evidência REAL, observável, não hipotética:
//       Grupo A (DEC-MOVE) -- um override GLOBAL de `::operator new`/`::operator delete`,
//       ESCOPADO por uma flag de observação + um casamento de tamanho EXATO (não um "alguma
//       alocação aconteceu" grosseiro), conta quantas alocações do tamanho de um buffer de pixel
//       decodificado distinto (não-potência-de-2, colisão improvável) acontecem durante UMA
//       chamada `decode_image_memory()`. O código consertado aloca exatamente UMA vez (o
//       `assign()` dentro de `decode_straight_rgba()`, `image_decode.hpp` -- irredutível sem
//       adotar o ponteiro dono do `malloc` do stb num `std::vector`, uma armadilha de UB de
//       alocadores incompatíveis que esta biblioteca deliberadamente não corre, ver o próprio
//       comentário de topo de `src/image.cpp`); o código com o bug de `const` aloca DUAS vezes (o
//       mesmo `assign()` MAIS a própria cópia-atribuição de `DecodedImagePixels::pixels`).
//       Determinístico, sem limite de memória de SO, sem depender da memória livre da máquina
//       hospedeira.
//       Grupo B (DEC-NOTHROW) -- um oráculo `fork()` + `setrlimit(RLIMIT_AS, ...)` de DOIS
//       ESTÁGIOS que força uma falha de alocação REAL dentro do caminho de decode (não um mock,
//       não uma hipótese), sobre um PNG PALETIZADO (indexado) feito à mão que casa com a própria
//       forma do repro do consumidor (ver o próprio comentário de topo de `build_indexed_png()`
//       pro porquê de ser paletizado especificamente). O ESTÁGIO 1/2 CALIBRA: um filho
//       não-restringido decodifica a MESMA fixture até o fim e reporta de volta, via um pipe,
//       `VmPeak - baseline` -- o crescimento REAL, medido, de pico de espaço de endereço virtual
//       que a chamada inteira precisou. O ESTÁGIO 2/2 APERTA: um segundo filho, restringido, seta
//       `RLIMIT_AS` pra `baseline + (pico medido * 3/4)`, confortavelmente abaixo do que o
//       ESTÁGIO 1/2 provou ser necessário, então ALGUMA alocação dentro da chamada precisa falhar
//       genuinamente. As próprias linhas de base dos dois filhos são medidas do próprio `VmSize`
//       de `/proc/self/status` deste processo (lida logo após cada `fork()`, antes de qualquer
//       alocação relacionada a decode), então o orçamento inteiro é dinâmico/auto-calibrado e não
//       depende da memória livre real da máquina hospedeira/CI (a própria lição "o teste atacava o
//       EXAGERO, não a FRONTEIRA" de `feedback_auditoria_domino` -- uma constante absoluta de
//       limite de memória ou nunca amarraria numa máquina com mais RAM, ou amarraria cedo/tarde
//       demais dependendo da própria linha de base de VmSize deste binário naquele host). O filho
//       apertado chama `decode_image_file()` ponta-a-ponta (a própria forma de chamada exata do
//       consumidor: um arquivo real em disco) e o PAI verifica, via `waitpid()`, que ele NÃO foi
//       morto por sinal (uma exceção não-capturada -> `std::terminate()` -> `abort()` -> `SIGABRT`
//       -- exatamente a regressão de DEC-NOTHROW que este teste existe pra pegar) e saiu de forma
//       limpa reportando `ok == false` (uma falha de alocação genuína, forçada, corretamente
//       degradada, não engolida silenciosamente num "sucesso" errado). Este desenho de dois
//       estágios MEDIR-e-então-APERTAR substituiu dois multiplicadores fixos anteriores, derivados
//       à mão (`baseline + 1,5*S`, depois `baseline + 1,75*S`, `S` o tamanho do buffer de pixel
//       decodificado) que os DOIS ficaram abaixo do pico real (medido, via teste de mutação
//       removendo o `try`/`catch`: o pico verdadeiro é próximo de 2,75*S, não os ~1,5-2*S que um
//       rastro à mão da própria sequência de alocação de `stb_image.h` previa) e apertavam tão
//       forte que ficavam falhando DENTRO do próprio `malloc()` do stb -- um caminho JÁ seguro que
//       continuava VERDE mesmo com o conserto removido, testando silenciosamente nada; ver o
//       próprio comentário de topo do bloco do Grupo B pra essa medição e a derivação completa.
//     Só Linux (`/proc/self/status`, `fork()`, `setrlimit()`) -- bate com o próprio escopo de
//     plataforma "Linux x86-64" de toda esta biblioteca (`CLAUDE.md`), nenhum `#ifdef`
//     necessário. Registrado INCONDICIONALMENTE (nenhuma dependência de display/GL/RmlUi -- mesma
//     racional "nada pra isolar aqui" de image_sanity.cpp/image_decode_sanity.cpp) -- roda nas
//     DUAS configurações de build.
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// ===========================================================================================
// EN: Group A support -- global allocation-count watch. See this file's own top comment for the
//     full rationale. `g_watching` gates the count so ONLY the code under test is measured (not
//     setup/teardown allocations); `g_watch_size` is the exact byte count to match.
// PT: Suporte do Grupo A -- observação global de contagem de alocação. Ver o próprio comentário
//     de topo deste arquivo pro racional completo. `g_watching` restringe a contagem pra que SÓ o
//     código sob teste seja medido (não alocações de setup/teardown); `g_watch_size` é a
//     contagem exata de bytes a casar.
// ===========================================================================================
bool g_watching = false;
std::size_t g_watch_size = 0;
int g_watch_count = 0;

} // namespace

// EN: Global operator new/delete override -- standard C++ technique for allocation-count
//     instrumentation (ODR-overrides the whole test binary's own new/delete; this test never
//     initialises RmlUi/GL/Draw2d/App/UiLayer, so the only large, size-distinctive allocations
//     that can occur are this test's own setup vectors and the decode path under test itself).
//     Delegates to std::malloc/std::free -- same underlying allocator libstdc++'s own default
//     `std::allocator` already uses, so this override changes NOTHING observable about actual
//     allocation behaviour, only adds counting.
// PT: Override global de operator new/delete -- técnica C++ padrão pra instrumentação de
//     contagem de alocação (ODR-sobrescreve o próprio new/delete do binário de teste inteiro;
//     este teste nunca inicializa RmlUi/GL/Draw2d/App/UiLayer, então as únicas alocações
//     grandes e de tamanho distintivo que podem ocorrer são os próprios vetores de setup deste
//     teste e o próprio caminho de decode sob teste). Delega pra std::malloc/std::free -- o
//     MESMO alocador subjacente que o próprio `std::allocator` default do libstdc++ já usa,
//     então este override não muda NADA observável sobre o comportamento de alocação real, só
//     acrescenta contagem.
void* operator new(std::size_t sz) {
  if (g_watching && sz == g_watch_size)
    ++g_watch_count;
  void* p = std::malloc(sz);
  if (p == nullptr)
    throw std::bad_alloc();
  return p;
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

// ===========================================================================================
// EN: Group B support -- read this process's own current virtual-address-space size (kB) from
//     /proc/self/status's VmSize field. Linux-only, matches this file's own top-comment platform
//     scope. Returns -1 on any parse failure (caller must treat that as "cannot run this group",
//     not as a zero baseline).
// PT: Suporte do Grupo B -- lê o próprio tamanho de espaço de endereço virtual (kB) deste
//     processo do campo VmSize de /proc/self/status. Só Linux, bate com o escopo de plataforma
//     do próprio comentário de topo deste arquivo. Devolve -1 em qualquer falha de parse (o
//     chamador precisa tratar isso como "não dá pra rodar este grupo", não como uma linha de
//     base zero).
// ===========================================================================================
long read_vm_size_bytes() {
  std::ifstream f("/proc/self/status");
  if (!f)
    return -1;
  std::string line;
  while (std::getline(f, line)) {
    if (line.compare(0, 7, "VmSize:") == 0) {
      std::istringstream iss(line.substr(7));
      long kb = -1;
      iss >> kb;
      if (kb < 0)
        return -1;
      return kb * 1024;
    }
  }
  return -1;
}

// EN: Builds a real, valid, solid-colour RGBA8 PNG of exactly `w * h` pixels via this library's
//     OWN encode_image_memory() (IMG-ENCODE, W21) -- self-contained, no external fixture file,
//     no hand-rolled PNG/zlib writer needed. `w`/`h` are DELIBERATELY non-round (not a clean
//     power of two) so the resulting `w * h * 4` byte count is unlikely to collide with any
//     incidental allocation the test harness or std library makes for unrelated reasons.
// PT: Constrói um PNG RGBA8 de cor sólida real, válido, de exatamente `w * h` pixels via o
//     PRÓPRIO encode_image_memory() desta biblioteca (IMG-ENCODE, W21) -- autocontido, nenhum
//     arquivo de fixture externo, nenhum escritor PNG/zlib feito à mão necessário. `w`/`h` são
//     DELIBERADAMENTE não-redondos (não uma potência de 2 limpa) pra que a contagem de bytes
//     `w * h * 4` resultante seja improvável de colidir com qualquer alocação incidental que o
//     harness de teste ou a biblioteca padrão façam por motivos não relacionados.
std::vector<unsigned char> build_solid_png(int w, int h) {
  std::vector<unsigned char> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
  for (std::size_t i = 0; i < pixels.size(); i += 4) {
    pixels[i + 0] = 120;
    pixels[i + 1] = 60;
    pixels[i + 2] = 200;
    pixels[i + 3] = 255;
  }
  const glintfx::EncodedImageBytes enc =
      glintfx::encode_image_memory(glintfx::ImageFormat::Png, w, h, pixels.data());
  return enc.ok ? enc.bytes : std::vector<unsigned char>{};
}

// ===========================================================================================
// EN: Group B's OWN fixture builder -- a hand-rolled, PALETTED (color_type=3, indexed) PNG with
//     a tRNS chunk, deliberately NOT built via this library's own encode_image_memory() (which
//     only ever WRITES truecolor/greyscale, never indexed -- stb_image_write has no palette
//     writer). Why paletted specifically, and why by hand: see this file's own top comment's
//     Group B rationale -- the consumer's OWN measured repro was a paletted PNG precisely
//     BECAUSE a paletted source's compressed/intermediate representation (1 byte/pixel index) is
//     SMALLER than its decoded RGBA8 output (4 bytes/pixel), the exact shape that makes stb's own
//     internal peak usage stay comfortably BELOW this library's own added copy -- see the
//     detailed allocation-sequence trace in the budget derivation comment at this function's own
//     call site below. CRC32 (chunk trailers) and the zlib Adler32 trailer are computed for real
//     (cheap, and good hygiene) even though `stb_image.h` itself does not verify EITHER of them
//     (confirmed by inspection: `stbi__parse_zlib_header()` validates the 2-byte zlib header
//     bytes and `cm == 8`, but nothing in this vendored copy touches CRC32 or Adler32) --
//     verified in the same commit against `third_party/stb/stb_image.h`. Deflate compression
//     itself is NOT implemented (would need a real Huffman/LZ77 encoder for no benefit here) --
//     the raw scanline bytes are wrapped in STORED (uncompressed) deflate blocks instead, a
//     first-class part of the DEFLATE spec every conforming inflater (including stb's own) must
//     support, and trivial to hand-construct (1 flag byte + LEN/NLEN + the literal bytes,
//     max 65535 bytes per block, hence the chunking loop below for a raw buffer that exceeds it).
// PT: Construtor de fixture PRÓPRIO do Grupo B -- um PNG PALETIZADO (color_type=3, indexado)
//     feito à mão, com um chunk tRNS, deliberadamente NÃO construído via o próprio
//     encode_image_memory() desta biblioteca (que só ESCREVE truecolor/greyscale, nunca
//     indexado -- o stb_image_write não tem escritor de paleta nenhum). Por que paletizado
//     especificamente, e por que à mão: ver o próprio racional do Grupo B no comentário de topo
//     deste arquivo -- o próprio repro medido pelo consumidor era um PNG paletizado precisamente
//     PORQUE a representação comprimida/intermediária de uma fonte paletizada (1 byte/pixel de
//     índice) é MENOR que a própria saída RGBA8 decodificada (4 bytes/pixel), exatamente a forma
//     que mantém o pico interno próprio do stb confortavelmente ABAIXO da própria cópia
//     adicionada por esta biblioteca -- ver o rastro detalhado de sequência de alocação no
//     comentário de derivação de orçamento no próprio sítio de chamada desta função abaixo.
//     CRC32 (trailers de chunk) e o trailer Adler32 do zlib são computados de verdade (barato, e
//     boa higiene) mesmo que o próprio `stb_image.h` não verifique NENHUM dos dois (confirmado
//     por inspeção: `stbi__parse_zlib_header()` valida os 2 bytes do header zlib e `cm == 8`, mas
//     nada nesta cópia vendorizada toca CRC32 ou Adler32) -- verificado no mesmo commit contra
//     `third_party/stb/stb_image.h`. A própria compressão deflate NÃO é implementada (precisaria
//     de um encoder Huffman/LZ77 de verdade sem benefício nenhum aqui) -- os bytes de scanline
//     crus são envolvidos em blocos deflate STORED (não-comprimidos) em vez disso, uma parte de
//     primeira classe da própria spec DEFLATE que todo inflater conforme (inclusive o próprio do
//     stb) precisa suportar, e trivial de construir à mão (1 byte de flag + LEN/NLEN + os bytes
//     literais, máximo 65535 bytes por bloco, daí o loop de fatiamento abaixo pra um buffer cru
//     que exceda isso).
// ===========================================================================================
std::uint32_t crc32_of(const unsigned char* buf, std::size_t len) {
  static std::uint32_t table[256];
  static bool table_ready = false;
  if (!table_ready) {
    for (std::uint32_t n = 0; n < 256; ++n) {
      std::uint32_t c = n;
      for (int k = 0; k < 8; ++k)
        c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      table[n] = c;
    }
    table_ready = true;
  }
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < len; ++i)
    crc = table[(crc ^ buf[i]) & 0xFFu] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

void put_be32(std::vector<unsigned char>& v, std::uint32_t x) {
  v.push_back(static_cast<unsigned char>((x >> 24) & 0xFFu));
  v.push_back(static_cast<unsigned char>((x >> 16) & 0xFFu));
  v.push_back(static_cast<unsigned char>((x >> 8) & 0xFFu));
  v.push_back(static_cast<unsigned char>(x & 0xFFu));
}

void put_png_chunk(std::vector<unsigned char>& out, const char (&tag)[5],
                   const std::vector<unsigned char>& data) {
  put_be32(out, static_cast<std::uint32_t>(data.size()));
  std::vector<unsigned char> tag_and_data(tag, tag + 4);
  tag_and_data.insert(tag_and_data.end(), data.begin(), data.end());
  out.insert(out.end(), tag_and_data.begin(), tag_and_data.end());
  put_be32(out, crc32_of(tag_and_data.data(), tag_and_data.size()));
}

std::vector<unsigned char> build_indexed_png(int w, int h) {
  // EN: Raw scanline buffer: 1 filter-type byte (0 == None) + w index bytes per row, every index
  //     == 0 (palette entry 0). Zero-initialised by the vector's own constructor -- both the
  //     filter byte and the index value we want happen to be 0, so no explicit fill needed.
  // PT: Buffer de scanline cru: 1 byte de filter-type (0 == None) + w bytes de índice por linha,
  //     todo índice == 0 (entrada 0 da paleta). Zero-inicializado pelo próprio construtor do
  //     vector -- tanto o byte de filtro quanto o valor de índice que queremos são 0, então
  //     nenhum preenchimento explícito é necessário.
  const std::vector<unsigned char> raw(static_cast<std::size_t>(h) * (1u + static_cast<std::size_t>(w)), 0);

  // EN: zlib stream: 2-byte header (validated by stb, see this function's top comment) + the raw
  //     data chunked into STORED deflate blocks (max 65535 bytes each) + a real Adler32 trailer.
  // PT: fluxo zlib: header de 2 bytes (validado pelo stb, ver o comentário de topo desta função)
  //     + o dado cru fatiado em blocos deflate STORED (máximo 65535 bytes cada) + um trailer
  //     Adler32 de verdade.
  std::vector<unsigned char> zdata;
  zdata.push_back(0x78);
  zdata.push_back(0x01); // (0x78*256 + 0x01) % 31 == 0, cm == 8 -- a valid zlib header.
  std::uint32_t adler_a = 1, adler_b = 0;
  std::size_t pos = 0;
  do {
    const std::size_t chunk = std::min<std::size_t>(65535u, raw.size() - pos);
    const bool last = (pos + chunk == raw.size());
    zdata.push_back(last ? 1u : 0u); // BFINAL(1 bit) + BTYPE=00 stored (2 bits), rest padding.
    const std::uint16_t len16 = static_cast<std::uint16_t>(chunk);
    const std::uint16_t nlen16 = static_cast<std::uint16_t>(~len16);
    zdata.push_back(static_cast<unsigned char>(len16 & 0xFFu));
    zdata.push_back(static_cast<unsigned char>((len16 >> 8) & 0xFFu));
    zdata.push_back(static_cast<unsigned char>(nlen16 & 0xFFu));
    zdata.push_back(static_cast<unsigned char>((nlen16 >> 8) & 0xFFu));
    zdata.insert(zdata.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos),
                 raw.begin() + static_cast<std::ptrdiff_t>(pos + chunk));
    for (std::size_t i = 0; i < chunk; ++i) {
      adler_a = (adler_a + raw[pos + i]) % 65521u;
      adler_b = (adler_b + adler_a) % 65521u;
    }
    pos += chunk;
  } while (pos < raw.size()); // EN: at least one block, even if raw were empty (it never is here).
  // PT: pelo menos um bloco, mesmo se raw fosse vazio (nunca é aqui).
  put_be32(zdata, (adler_b << 16) | adler_a);

  std::vector<unsigned char> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

  std::vector<unsigned char> ihdr;
  put_be32(ihdr, static_cast<std::uint32_t>(w));
  put_be32(ihdr, static_cast<std::uint32_t>(h));
  ihdr.push_back(8); // bit depth
  ihdr.push_back(3); // color type 3 == indexed/paletted
  ihdr.push_back(0); // compression method (only 0 is defined)
  ihdr.push_back(0); // filter method (only 0 is defined)
  ihdr.push_back(0); // interlace method (0 == none)
  put_png_chunk(png, "IHDR", ihdr);

  // EN: 2 palette entries -- only entry 0 (used by every pixel) needs to be meaningful; entry 1
  //     is filler so PLTE's own length (2 entries) safely covers tRNS's own 1-byte length check
  //     (`c.length > pal_len` in stb_image.h -- see this function's top comment).
  // PT: 2 entradas de paleta -- só a entrada 0 (usada por todo pixel) precisa ser significativa;
  //     a entrada 1 é enchimento pra que o próprio comprimento de PLTE (2 entradas) cubra com
  //     segurança a checagem de comprimento de 1 byte do próprio tRNS (`c.length > pal_len` em
  //     stb_image.h -- ver o comentário de topo desta função).
  const std::vector<unsigned char> plte = {120, 60, 200, 10, 10, 10};
  put_png_chunk(png, "PLTE", plte);

  // EN: tRNS with 1 alpha byte for palette entry 0 -- this is what makes stb_image.h set
  //     `pal_img_n = 4` (see this function's top comment) instead of 3, so the palette-expand
  //     step below produces RGBA directly without a SECOND, separate 3->4 conversion pass.
  // PT: tRNS com 1 byte de alpha pra entrada 0 da paleta -- é isto que faz o stb_image.h setar
  //     `pal_img_n = 4` (ver o comentário de topo desta função) em vez de 3, pra que o passo de
  //     expansão de paleta abaixo produza RGBA diretamente sem uma SEGUNDA passada de conversão
  //     3->4 separada.
  const std::vector<unsigned char> trns = {255};
  put_png_chunk(png, "tRNS", trns);

  put_png_chunk(png, "IDAT", zdata);
  put_png_chunk(png, "IEND", std::vector<unsigned char>{});
  return png;
}

} // namespace

using namespace glintfx;

int main() {
  // ===========================================================================================
  // EN: Group A (DEC-MOVE) -- allocation-COUNT oracle. See this file's top comment for the full
  //     rationale. `kW`/`kH` chosen so `kW * kH * 4` (55,348 bytes) is an odd-enough number that
  //     it will not collide with any other allocation this small, self-contained test process
  //     makes (std::string/std::vector internal growth chunks are typically small powers of two
  //     or cache-line multiples, not this).
  // PT: Grupo A (DEC-MOVE) -- oráculo de CONTAGEM de alocação. Ver o comentário de topo deste
  //     arquivo pro racional completo. `kW`/`kH` escolhidos pra que `kW * kH * 4` (55.348 bytes)
  //     seja um número ímpar o bastante pra não colidir com nenhuma outra alocação que este
  //     processo de teste pequeno e autocontido faça (blocos de crescimento interno de
  //     std::string/std::vector são tipicamente potências de 2 pequenas ou múltiplos de linha de
  //     cache, não isto).
  // ===========================================================================================
  {
    const int kW = 137;
    const int kH = 101;
    const std::size_t expect_bytes = static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4u;
    check(expect_bytes == 55348u, "Group A setup: expected pixel byte count is the chosen 55348");

    const std::vector<unsigned char> png = build_solid_png(kW, kH);
    check(!png.empty(), "Group A setup: build_solid_png() produced a real encoded PNG");

    if (!png.empty()) {
      g_watch_size = expect_bytes;
      g_watch_count = 0;
      g_watching = true;
      const DecodedImagePixels decoded = decode_image_memory(png.data(), png.size());
      g_watching = false;

      check(decoded.ok, "Group A: decode_image_memory() succeeded on the self-built PNG");
      check(decoded.pixels.size() == expect_bytes,
            "Group A: decoded pixel buffer is the expected width*height*4 byte count");
      // EN: THE oracle -- exactly ONE allocation of the pixel-buffer's exact size (the
      //     irreducible assign() inside decode_straight_rgba()). Before the DEC-MOVE fix this
      //     would be TWO: that same assign() PLUS DecodedImagePixels::pixels's own
      //     copy-assignment (the const-defeated std::move()). See this file's top comment.
      // PT: O oráculo -- exatamente UMA alocação do tamanho exato do buffer de pixel (o assign()
      //     irredutível dentro de decode_straight_rgba()). Antes do fix DEC-MOVE seriam DUAS:
      //     aquele mesmo assign() MAIS a própria cópia-atribuição de
      //     DecodedImagePixels::pixels (o std::move() derrotado por const). Ver o comentário de
      //     topo deste arquivo.
      char msg[160];
      std::snprintf(msg, sizeof(msg),
                    "Group A (DEC-MOVE): exactly 1 allocation of size %zu during "
                    "decode_image_memory() -- got %d (2 would mean the const-move copy is back)",
                    expect_bytes, g_watch_count);
      check(g_watch_count == 1, msg);
    }
  }

  // ===========================================================================================
  // EN: Group B (DEC-NOTHROW) -- forced-allocation-failure oracle via fork()+RLIMIT_AS. The
  //     ACTUAL budget is computed empirically, in two stages, right below (STAGE 1/2 CALIBRATE,
  //     STAGE 2/2 SQUEEZE) -- see those two comments for the current, robust derivation. This
  //     block's own history (kept here because it explains why `build_indexed_png()` -- a
  //     hand-rolled PALETTED PNG, not a plain truecolor one -- exists at all): a first attempt
  //     hand-traced `third_party/stb/stb_image.h`'s own PNG decode path for a paletted
  //     (color_type=3) + tRNS source, `S = w*h*4` the final decoded size, and predicted two
  //     distinct peaks -- ~1.5*S transiently inside stb's own `stbi__expand_png_palette()` (plain
  //     `malloc()`, never throws) and 2*S once this library's own `assign()`
  //     (`image_decode.hpp`) copies the final `px` buffer into `out.rgba` (a C++ allocation that
  //     DOES throw `std::bad_alloc`) -- and picked `baseline + 1.75*S` as the midpoint. Measured
  //     via mutation testing (removing the `try`/`catch`, this file's own top comment): the real
  //     peak turned out to be close to 2.75*S (VmPeak - baseline), not ~1.5-2*S, so that budget
  //     squeezed so tight it kept failing INSIDE stb's own `malloc()` -- an ALREADY-safe path that
  //     stayed GREEN even with the `try`/`catch` removed, silently testing nothing. The paletted
  //     source itself is still the right choice (it is why a REAL crash was reproducible at all
  //     once the budget was fixed by measuring instead of guessing -- see STAGE 1/2 below); only
  //     the fixed-multiplier ARITHMETIC was wrong.
  // PT: Grupo B (DEC-NOTHROW) -- oráculo de falha-de-alocação-forçada via fork()+RLIMIT_AS. O
  //     orçamento REAL é computado empiricamente, em dois estágios, logo abaixo (ESTÁGIO 1/2
  //     CALIBRAR, ESTÁGIO 2/2 APERTAR) -- ver os dois comentários pra derivação atual, robusta.
  //     O histórico deste bloco (mantido aqui porque explica por que `build_indexed_png()` -- um
  //     PNG PALETIZADO feito à mão, não um truecolor simples -- existe): uma primeira tentativa
  //     rastreou à mão o próprio caminho de decode PNG de `third_party/stb/stb_image.h` pra uma
  //     fonte paletizada (color_type=3) + tRNS, `S = w*h*4` o tamanho decodificado final, e
  //     previu dois picos distintos -- ~1,5*S transitoriamente dentro do próprio
  //     `stbi__expand_png_palette()` do stb (`malloc()` puro, nunca lança) e 2*S quando o próprio
  //     `assign()` desta biblioteca (`image_decode.hpp`) copia o buffer `px` final pra `out.rgba`
  //     (uma alocação C++ que LANÇA `std::bad_alloc`) -- e escolheu `baseline + 1,75*S` como
  //     ponto médio. Medido via teste de mutação (removendo o `try`/`catch`, ver o próprio
  //     comentário de topo deste arquivo): o pico real acabou sendo próximo de 2,75*S
  //     (VmPeak - baseline), não ~1,5-2*S, então aquele orçamento apertava tão forte que ficava
  //     falhando DENTRO do próprio `malloc()` do stb -- um caminho JÁ seguro que continuava VERDE
  //     mesmo com o `try`/`catch` removido, testando silenciosamente nada. A própria fonte
  //     paletizada segue sendo a escolha certa (é o que tornou um crash REAL reproduzível assim
  //     que o orçamento foi corrigido medindo em vez de chutar -- ver o ESTÁGIO 1/2 abaixo); só a
  //     ARITMÉTICA de multiplicador fixo estava errada.
  // ===========================================================================================
  {
    const int kW = 2560;
    const int kH = 2048;
    const std::size_t s_bytes = static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4u;
    check(s_bytes == 20u * 1024u * 1024u, "Group B setup: expected decoded size is exactly 20 MiB");

    const std::vector<unsigned char> png = build_indexed_png(kW, kH);
    check(!png.empty(), "Group B setup: build_indexed_png() produced a real encoded PNG");

    char tmpl[] = "/var/tmp/glintfx_decode_hardening_XXXXXX";
    const int fd = mkstemp(tmpl);
    bool have_file = false;
    if (fd >= 0 && !png.empty()) {
      std::FILE* f = fdopen(fd, "wb");
      if (f) {
        have_file = std::fwrite(png.data(), 1, png.size(), f) == png.size();
        std::fclose(f);
      } else {
        close(fd);
      }
    }
    check(have_file, "Group B setup: wrote the self-built PNG to a scratch file on /var/tmp");

    if (have_file) {
      // ===========================================================================================
      // EN: STAGE 1/2 -- CALIBRATE. An unconstrained child runs `decode_image_file()` to
      //     completion (must succeed -- no memory pressure at all) and reports back, via a pipe,
      //     `VmPeak - baseline` -- the REAL, measured total virtual-address-space growth the
      //     WHOLE call needed at its single highest point. This replaces an earlier, hand-derived
      //     fixed multiplier (`baseline + 1.75*S`, reasoned from a manual trace of
      //     `stb_image.h`'s own allocation sequence -- see this block's own history in git blame)
      //     that undershot the REAL peak by a wide margin (measured: the actual peak is close to
      //     2.75x the decoded pixel-buffer size S, not the ~1.5-2x the hand trace predicted --
      //     `z->idata`/`z->expanded`/page-rounding/malloc-arena overhead this repo's own trace did
      //     not fully account for) and, as a result, squeezed so tight that stb's OWN internal
      //     `malloc()` failed FIRST every time -- an ALREADY-safe path (no C++ exception, no throw)
      //     that would pass this test even WITHOUT the DEC-NOTHROW fix, silently testing nothing.
      //     A mutation test (removing the `try`/`catch`, see this file's own top comment) exposed
      //     this: it stayed GREEN when it should have gone red. Measuring the REAL peak, rather
      //     than re-deriving a smarter multiplier from an incomplete source trace, is the
      //     ROBUST fix -- it does not depend on correctly enumerating every internal buffer a
      //     vendored third-party decoder allocates, only on what actually happened.
      // PT: ESTÁGIO 1/2 -- CALIBRAR. Um filho não-restringido roda `decode_image_file()` até o
      //     fim (precisa ter sucesso -- nenhuma pressão de memória) e reporta de volta, via um
      //     pipe, `VmPeak - baseline` -- o crescimento REAL, medido, de espaço de endereço
      //     virtual que a chamada INTEIRA precisou no próprio ponto mais alto. Isto substitui um
      //     multiplicador fixo anterior, derivado à mão (`baseline + 1,75*S`, raciocinado a partir
      //     de um rastro manual da própria sequência de alocação de `stb_image.h` -- ver o próprio
      //     histórico deste bloco no git blame) que ficou bem abaixo do pico REAL (medido: o pico
      //     real é próximo de 2,75x o tamanho do buffer de pixel decodificado S, não os ~1,5-2x
      //     que o rastro à mão previa -- sobrecarga de `z->idata`/`z->expanded`/arredondamento-de-
      //     página/arena-de-malloc que o próprio rastro deste repo não contabilizou por completo)
      //     e, como resultado, apertava tão forte que o próprio `malloc()` interno do stb falhava
      //     PRIMEIRO toda vez -- um caminho JÁ seguro (sem exceção C++, sem lançamento) que
      //     passaria neste teste MESMO SEM o conserto DEC-NOTHROW, testando silenciosamente nada.
      //     Um teste de mutação (removendo o `try`/`catch`, ver o próprio comentário de topo deste
      //     arquivo) expôs isto: continuou VERDE quando devia ter ficado vermelho. Medir o pico
      //     REAL, em vez de re-derivar um multiplicador mais esperto a partir de um rastro de
      //     fonte incompleto, é o conserto ROBUSTO -- não depende de enumerar corretamente cada
      //     buffer interno que um decoder de terceiro vendorizado aloca, só do que realmente
      //     aconteceu.
      // ===========================================================================================
      int cal_pipe[2] = {-1, -1};
      long cal_delta_bytes = -1;
      bool cal_ok = false;
      if (pipe(cal_pipe) == 0) {
        const pid_t cal_pid = fork();
        check(cal_pid >= 0, "Group B calibration: fork() succeeded");
        if (cal_pid == 0) {
          close(cal_pipe[0]);
          const long b = read_vm_size_bytes();
          const DecodedImagePixels decoded = decode_image_file(tmpl); // unconstrained -- must succeed.
          long peak_kb = -1;
          std::ifstream f2("/proc/self/status");
          std::string line2;
          while (std::getline(f2, line2)) {
            if (line2.compare(0, 7, "VmPeak:") == 0) {
              std::istringstream iss(line2.substr(7));
              iss >> peak_kb;
              break;
            }
          }
          long delta = (b >= 0 && peak_kb >= 0 && decoded.ok) ? (peak_kb * 1024 - b) : -1;
          const ssize_t written = write(cal_pipe[1], &delta, sizeof(delta));
          (void)written;
          close(cal_pipe[1]);
          _exit(0);
        }
        close(cal_pipe[1]);
        long delta = -1;
        const ssize_t got = read(cal_pipe[0], &delta, sizeof(delta));
        close(cal_pipe[0]);
        int cal_status = 0;
        waitpid(cal_pid, &cal_status, 0);
        if (got == static_cast<ssize_t>(sizeof(delta)) && delta > 0) {
          cal_delta_bytes = delta;
          cal_ok = true;
        }
      }
      check(cal_ok,
            "Group B calibration: measured a real, positive peak-usage delta for an "
            "unconstrained decode_image_file() call");

      if (!cal_ok) {
        std::fprintf(stderr,
                     "Group B: SKIPPED -- calibration failed (pipe/fork/proc read "
                     "issue?) -- inconclusive, not a failure\n");
      } else {
        const pid_t pid = fork();
        check(pid >= 0, "Group B: fork() succeeded");

        if (pid == 0) {
          // EN: STAGE 2/2 -- SQUEEZE. `baseline + cal_delta_bytes - margin`, where `margin` is a
          //     fraction (1/4, see the derivation right below on the `margin` line itself) of the
          //     CALIBRATED delta itself (not of `s_bytes` -- scales with whatever the real peak
          //     turned out to be, on THIS host/build, not a guess). This guarantees the constrained
          //     run falls short of what the calibration run proved was needed for a full,
          //     successful decode -- so SOME allocation inside the call must fail. See this block's
          //     own top comment for why this is a more robust guarantee than re-deriving a smarter
          //     fixed multiplier.
          // PT: ESTÁGIO 2/2 -- APERTAR. `baseline + cal_delta_bytes - margin`, onde `margin` é uma
          //     fração (1/4, ver a derivação logo abaixo na própria linha do `margin`) do próprio
          //     delta CALIBRADO (não de `s_bytes` -- escala com o que quer que o pico real tenha
          //     sido, NESTE host/build, não um chute). Isto garante que a execução restringida fica
          //     abaixo do que a execução de calibração provou ser
          //     necessário pra um decode completo e bem-sucedido -- então ALGUMA alocação dentro da
          //     chamada precisa falhar. Ver o próprio comentário de topo deste bloco pro porquê
          //     disto ser uma garantia mais robusta que re-derivar um multiplicador fixo mais
          //     esperto.
          const long baseline = read_vm_size_bytes();
          if (baseline < 0)
            _exit(2); // EN: cannot measure -- report distinctly, parent treats as inconclusive.
                      // PT: não dá pra medir -- reporta distinto, o pai trata como inconclusivo.

          // EN: 1/4 of the CALIBRATED delta -- measured empirically (see this block's own top
          //     comment) to reliably force a genuine allocation failure: a smaller margin (1/16,
          //     tried first) let run-to-run variance (malloc arena reuse, ASLR) occasionally let the
          //     constrained run "unexpectedly fit" (exit 0, no failure forced at all -- a different,
          //     also-wrong outcome from squeezing too tight into stb's own internal allocation).
          // PT: 1/4 do delta CALIBRADO -- medido empiricamente (ver o próprio comentário de topo
          //     deste bloco) pra forçar de forma confiável uma falha de alocação genuína: uma
          //     margem menor (1/16, tentada primeiro) deixava a variância de execução-a-execução
          //     (reuso de arena do malloc, ASLR) ocasionalmente deixar a execução restringida
          //     "caber inesperadamente" (saída 0, nenhuma falha forçada de jeito nenhum -- um
          //     resultado diferente, também errado, de apertar demais dentro da própria alocação
          //     interna do stb).
          const long margin = cal_delta_bytes / 4;
          const rlim_t limit = static_cast<rlim_t>(baseline) +
                               static_cast<rlim_t>(cal_delta_bytes - margin);
          struct rlimit rl;
          rl.rlim_cur = limit;
          rl.rlim_max = limit;
          if (setrlimit(RLIMIT_AS, &rl) != 0)
            _exit(3); // EN: could not clamp -- inconclusive, not a pass/fail either way.
                      // PT: não deu pra apertar -- inconclusivo, nem passa nem falha.

          // EN: THE call under test -- decode_image_file() end-to-end, the consumer's own exact
          //     shape (a real file on disk). If DEC-NOTHROW regresses (the try/catch removed, or
          //     the allocation genuinely escapes uncaught), std::bad_alloc propagates out of main()
          //     uncaught -> std::terminate() -> abort() -> this process dies by SIGABRT, and the
          //     parent's WIFSIGNALED() check below catches it.
          // PT: A chamada sob teste -- decode_image_file() ponta-a-ponta, a própria forma exata do
          //     consumidor (um arquivo real em disco). Se DEC-NOTHROW regredir (o try/catch
          //     removido, ou a alocação genuinamente escapar não-capturada), std::bad_alloc se
          //     propaga pra fora do main() não-capturada -> std::terminate() -> abort() -> este
          //     processo morre por SIGABRT, e a checagem WIFSIGNALED() do pai abaixo pega isso.
          const DecodedImagePixels decoded = decode_image_file(tmpl);
          _exit(decoded.ok ? 0 : 1); // 0 = unexpectedly succeeded despite the squeeze; 1 = ok==false as expected.
        }

        int status = 0;
        const pid_t waited = waitpid(pid, &status, 0);
        check(waited == pid, "Group B: waitpid() reaped the forked child");

        const bool signalled = WIFSIGNALED(status);
        char sig_msg[160];
        std::snprintf(sig_msg, sizeof(sig_msg),
                      "Group B (DEC-NOTHROW): child was NOT killed by a signal (uncaught "
                      "std::bad_alloc would SIGABRT) -- WIFSIGNALED=%d, signal=%d",
                      signalled ? 1 : 0, signalled ? WTERMSIG(status) : 0);
        check(!signalled, sig_msg);

        if (!signalled && WIFEXITED(status)) {
          const int code = WEXITSTATUS(status);
          if (code == 2) {
            std::fprintf(stderr,
                         "Group B: SKIPPED oracle body -- child could not read /proc/self/status "
                         "VmSize (non-Linux or unreadable /proc?) -- inconclusive, not a failure\n");
          } else if (code == 3) {
            std::fprintf(stderr,
                         "Group B: SKIPPED oracle body -- child's setrlimit(RLIMIT_AS) failed "
                         "(sandboxed/restricted environment?) -- inconclusive, not a failure\n");
          } else {
            check(code == 1,
                  "Group B: decode_image_file() reported ok == false under the forced squeeze "
                  "(exit 1 expected; 0 means the allocation unexpectedly fit -- widen the budget)");
          }
        }
      } // EN: end of `if (cal_ok)`. PT: fim do `if (cal_ok)`.
    }

    std::remove(tmpl);
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "image_decode_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("image_decode_hardening_sanity: PASS");
  return 0;
}
