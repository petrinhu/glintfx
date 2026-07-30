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
//       Group B (DEC-NOTHROW) -- a `fork()` + `setrlimit(RLIMIT_AS, ...)` oracle that forces a
//       REAL allocation failure inside the decode path (not a mock, not a hypothetical): the
//       budget is computed from THIS process's OWN measured `/proc/self/status` `VmSize` baseline
//       (read in the child, right after `fork()`, before any decode-related allocation) plus
//       1.5x a chosen pixel-buffer size S -- so the FIRST allocation (stb's `malloc`'d decode
//       buffer, size S) fits, but the SECOND (this library's own `std::vector` copy, ALSO size S,
//       transiently alive at the SAME TIME as the first, inside `decode_straight_rgba()`) does
//       not (baseline + 2S > baseline + 1.5S). This is dynamic/self-calibrating specifically so
//       it does not depend on the host/CI machine's actual free memory (`feedback_
//       auditoria_domino`'s own "the test attacked the EXAGGERATION, not the BOUNDARY" lesson --
//       an absolute memory-limit constant would either never bind on a machine with more RAM, or
//       bind too early/late depending on this binary's own baseline VmSize on that host) -- the
//       SAME reason the OOM budget is measured, not hardcoded. The child calls
//       `decode_image_file()` end-to-end (the consumer's own exact call shape: a real file on
//       disk) and the PARENT asserts, via `waitpid()`, that the child was NOT killed by a signal
//       (an uncaught exception -> `std::terminate()` -> `abort()` -> `SIGABRT` -- exactly the
//       DEC-NOTHROW regression this test exists to catch) and exited cleanly reporting
//       `ok == false` (a genuine, forced allocation failure correctly degraded, not silently
//       swallowed into a wrong "success").
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
//       Grupo B (DEC-NOTHROW) -- um oráculo `fork()` + `setrlimit(RLIMIT_AS, ...)` que força uma
//       falha de alocação REAL dentro do caminho de decode (não um mock, não uma hipótese): o
//       orçamento é computado a partir da PRÓPRIA linha de base medida deste processo em
//       `/proc/self/status` `VmSize` (lida no filho, logo após o `fork()`, antes de qualquer
//       alocação relacionada a decode) mais 1,5x um tamanho de buffer de pixel escolhido S -- pra
//       que a PRIMEIRA alocação (o buffer de decode `malloc`'d do stb, tamanho S) caiba, mas a
//       SEGUNDA (a própria cópia `std::vector` desta biblioteca, TAMBÉM tamanho S, viva
//       transitoriamente ao MESMO TEMPO que a primeira, dentro de `decode_straight_rgba()`) não
//       caiba (baseline + 2S > baseline + 1.5S). Isto é dinâmico/auto-calibrado especificamente
//       pra NÃO depender da memória livre real da máquina hospedeira/CI (a própria lição "o teste
//       atacava o EXAGERO, não a FRONTEIRA" de `feedback_auditoria_domino` -- uma constante
//       absoluta de limite de memória ou nunca amarraria numa máquina com mais RAM, ou amarraria
//       cedo/tarde demais dependendo da própria linha de base de VmSize deste binário naquele
//       host) -- o MESMO motivo pelo qual o orçamento é medido, não fixado. O filho chama
//       `decode_image_file()` ponta-a-ponta (a própria forma de chamada exata do consumidor: um
//       arquivo real em disco) e o PAI verifica, via `waitpid()`, que o filho NÃO foi morto por
//       sinal (uma exceção não-capturada -> `std::terminate()` -> `abort()` -> `SIGABRT` --
//       exatamente a regressão de DEC-NOTHROW que este teste existe pra pegar) e saiu de forma
//       limpa reportando `ok == false` (uma falha de alocação genuína, forçada, corretamente
//       degradada, não engolida silenciosamente num "sucesso" errado).
//     Só Linux (`/proc/self/status`, `fork()`, `setrlimit()`) -- bate com o próprio escopo de
//     plataforma "Linux x86-64" de toda esta biblioteca (`CLAUDE.md`), nenhum `#ifdef`
//     necessário. Registrado INCONDICIONALMENTE (nenhuma dependência de display/GL/RmlUi -- mesma
//     racional "nada pra isolar aqui" de image_sanity.cpp/image_decode_sanity.cpp) -- roda nas
//     DUAS configurações de build.
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

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
  // EN: Group B (DEC-NOTHROW) -- forced-allocation-failure oracle via fork()+RLIMIT_AS. See this
  //     file's top comment for the full rationale and the baseline/1.5x-budget derivation.
  // PT: Grupo B (DEC-NOTHROW) -- oráculo de falha-de-alocação-forçada via fork()+RLIMIT_AS. Ver o
  //     comentário de topo deste arquivo pro racional completo e a derivação de linha-de-
  //     base/orçamento 1,5x.
  // ===========================================================================================
  {
    const int kW = 2560;
    const int kH = 2048;
    const std::size_t s_bytes = static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4u;
    check(s_bytes == 20u * 1024u * 1024u, "Group B setup: expected decoded size is exactly 20 MiB");

    const std::vector<unsigned char> png = build_solid_png(kW, kH);
    check(!png.empty(), "Group B setup: build_solid_png() produced a real encoded PNG");

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
      const pid_t pid = fork();
      check(pid >= 0, "Group B: fork() succeeded");

      if (pid == 0) {
        // EN: CHILD -- measure baseline, clamp RLIMIT_AS, force the allocation failure, report
        //     the outcome via exit code (never via shared memory/IPC -- exit status is the
        //     simplest channel that survives an abort()'d process too, since WIFSIGNALED itself
        //     is the primary oracle).
        // PT: FILHO -- mede a linha de base, aperta RLIMIT_AS, força a falha de alocação, reporta
        //     o resultado via código de saída (nunca via memória compartilhada/IPC -- código de
        //     saída é o canal mais simples que sobrevive a um processo abortado também, já que o
        //     próprio WIFSIGNALED é o oráculo primário).
        const long baseline = read_vm_size_bytes();
        if (baseline < 0)
          _exit(2); // EN: cannot measure -- report distinctly, parent treats as inconclusive.
                    // PT: não dá pra medir -- reporta distinto, o pai trata como inconclusivo.

        const rlim_t limit =
            static_cast<rlim_t>(baseline) + static_cast<rlim_t>(s_bytes) + static_cast<rlim_t>(s_bytes / 2);
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
