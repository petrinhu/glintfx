// SPDX-License-Identifier: Apache-2.0
// EN: ENC-NOTHROW, W21 2026-07-30 -- gêmeo do DEC-NOTHROW (`image.cpp`/
//     `image_decode_hardening_sanity.cpp`) para o par PÚBLICO de encode
//     (`encode_image_memory()`/`encode_image_file()`, `glintfx/image.hpp`): o header promete
//     "never a crash" com as MESMAS palavras que prometia pro decode -- achado de auditoria-dominó
//     do próprio team lead, aceito e corrigido na mesma release por coerência de tag (v0.27.0 não
//     pode anunciar "o decode não lança mais" enquanto o encode, entregue na MESMA release,
//     lança). Prova o conserto com o MESMO oráculo de dois estágios do DEC-NOTHROW (fork()+
//     RLIMIT_AS calibrado por medição real, não por multiplicador fixo derivado à mão -- ver
//     `image_decode_hardening_sanity.cpp`'s own top comment for the exact story of why a
//     hand-derived fixed budget silently tested nothing there), agora sobre `encode_image_file()`
//     ponta-a-ponta (pixels em memória -> arquivo em disco).
//
//     POR QUE BMP, NÃO PNG (o próprio ponto que o team lead levantou: "o pico do encode depende
//     do formato -- o PNG comprime, o BMP não"): PNG comprime a saída pra uma fração pequena do
//     buffer de pixel de entrada, então o PICO do encode inteiro fica dominado quase inteiramente
//     pelo PRÓPRIO buffer de pixel de entrada (que o CHAMADOR já possui, fora do controle desta
//     API) -- a margem entre "cabe" e "não cabe" fica estreita e domina de fora, difícil de
//     separar do que a API em si aloca. BMP é SEM COMPRESSÃO: `write_cb` cresce
//     `EncodedImageBytes::bytes` até aproximadamente o MESMO tamanho do buffer de pixel de
//     entrada (`width * height * 4`, já que todo formato aqui recebe `comp=4`) -- então o pico
//     real do encode fica em ~2x o tamanho do buffer de pixel (entrada + saída vivos ao mesmo
//     tempo), uma margem previsível e separável, exatamente o efeito que o team lead descreveu.
//
//     TWO GROUPS, an adversarial-review finding (W21): Group 1 (the `RLIMIT_AS` oracle above)
//     proves the WHOLE `encode_image_file()` call degrades cleanly under real memory pressure,
//     but structurally CANNOT discriminate `encode_image_file()`'s OWN `try`/`catch`
//     specifically -- proven, not assumed, by mutation: removing ONLY that `try` (keeping
//     `encode_image_memory()`'s own intact) left Group 1 GREEN, because `encode_image_memory()`
//     is itself `noexcept` with its own `try`, absorbing any allocation failure before it ever
//     reaches `encode_image_file()`'s frame. Group 2 (below) closes that gap with a DIFFERENT
//     technique -- size-targeted `operator new` injection, not a memory budget -- specifically
//     isolating the `std::ofstream`'s own internal buffer allocation. See Group 2's own top
//     comment (in `main()`) for the full rationale.
//
//     Linux-only (`/proc/self/status`, `fork()`, `setrlimit()`), mesmo escopo de plataforma que
//     `image_decode_hardening_sanity.cpp` já declara -- nenhum `#ifdef` necessário. Registrado
//     UNCONDICIONALMENTE (nenhuma dependência de display/GL/RmlUi) -- roda nas DUAS configurações
//     de build.
// PT: ENC-NOTHROW, W21 2026-07-30 -- gêmeo do DEC-NOTHROW (`image.cpp`/
//     `image_decode_hardening_sanity.cpp`) para o par PÚBLICO de encode
//     (`encode_image_memory()`/`encode_image_file()`, `glintfx/image.hpp`): o header promete
//     "nunca um crash" com as MESMAS palavras que prometia pro decode -- achado de auditoria-
//     dominó do próprio team lead, aceito e corrigido na mesma release por coerência de tag
//     (a v0.27.0 não pode anunciar "o decode não lança mais" enquanto o encode, entregue na MESMA
//     release, lança). Prova o conserto com o MESMO oráculo de dois estágios do DEC-NOTHROW
//     (fork()+RLIMIT_AS calibrado por medição real, não por multiplicador fixo derivado à mão --
//     ver o próprio comentário de topo de `image_decode_hardening_sanity.cpp` pra história exata
//     de por que um orçamento fixo derivado à mão testava silenciosamente nada lá), agora sobre
//     `encode_image_file()` ponta-a-ponta (pixels em memória -> arquivo em disco).
//
//     POR QUE BMP, NÃO PNG (o próprio ponto que o team lead levantou: "o pico do encode depende
//     do formato -- o PNG comprime, o BMP não"): PNG comprime a saída pra uma fração pequena do
//     buffer de pixel de entrada, então o PICO do encode inteiro fica dominado quase inteiramente
//     pelo PRÓPRIO buffer de pixel de entrada (que o CHAMADOR já possui, fora do controle desta
//     API) -- a margem entre "cabe" e "não cabe" fica estreita e domina de fora, difícil de
//     separar do que a API em si aloca. BMP é SEM COMPRESSÃO: `write_cb` cresce
//     `EncodedImageBytes::bytes` até aproximadamente o MESMO tamanho do buffer de pixel de entrada
//     (`width * height * 4`, já que todo formato aqui recebe `comp=4`) -- então o pico real do
//     encode fica em ~2x o tamanho do buffer de pixel (entrada + saída vivos ao mesmo tempo), uma
//     margem previsível e separável, exatamente o efeito que o team lead descreveu.
//
//     DOIS GRUPOS, um achado de review adversarial (W21): o Grupo 1 (o oráculo `RLIMIT_AS` acima)
//     prova que a chamada INTEIRA a `encode_image_file()` degrada limpamente sob pressão de
//     memória real, mas ESTRUTURALMENTE NÃO CONSEGUE discriminar o PRÓPRIO `try`/`catch` de
//     `encode_image_file()` especificamente -- provado, não assumido, por mutação: remover SÓ
//     aquele `try` (mantendo o próprio de `encode_image_memory()` intacto) deixou o Grupo 1 VERDE,
//     porque `encode_image_memory()` em si é `noexcept` com o próprio `try`, absorvendo qualquer
//     falha de alocação antes dela sequer alcançar o quadro de `encode_image_file()`. O Grupo 2
//     (abaixo) fecha essa lacuna com uma técnica DIFERENTE -- injeção de `operator new` mirada por
//     tamanho, não um orçamento de memória -- isolando especificamente a própria alocação de
//     buffer interno do `std::ofstream`. Ver o próprio comentário de topo do Grupo 2 (em `main()`)
//     pro racional completo.
//
//     Só Linux (`/proc/self/status`, `fork()`, `setrlimit()`), mesmo escopo de plataforma que
//     `image_decode_hardening_sanity.cpp` já declara -- nenhum `#ifdef` necessário. Registrado
//     INCONDICIONALMENTE (nenhuma dependência de display/GL/RmlUi) -- roda nas DUAS configurações
//     de build.
// Copyright (c) 2026 Petrus Silva Costa
#include "glintfx/image.hpp"

#include <cstdio>
#include <cstdlib>
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
// EN: Group 2 support (ofstream-specific size-injection oracle) -- see main()'s own Group 2 top
//     comment for the full rationale. POD, zero-initialised by constant initialisation (no
//     dynamic init-order risk, unlike a `std::vector` here would carry): safe to read/write from
//     inside `operator new` itself, which may run before any other global's own constructor.
// PT: Suporte do Grupo 2 (oráculo de injeção de falha por tamanho, específico do ofstream) -- ver
//     o próprio comentário de topo do Grupo 2 em main() pro racional completo. POD,
//     zero-inicializado por inicialização constante (sem risco de ordem de inicialização dinâmica,
//     ao contrário do que um `std::vector` aqui carregaria): seguro de ler/escrever de dentro do
//     próprio `operator new`, que pode rodar antes do próprio construtor de qualquer outro global.
// ===========================================================================================
constexpr int kMaxSeenSizes = 64;
std::size_t g_seen_sizes[kMaxSeenSizes];
int g_seen_count = 0;
bool g_watching = false;
bool g_armed = false;
std::size_t g_fail_size = 0;

} // namespace

// EN: Global operator new/delete override -- same standard technique
//     `image_decode_hardening_sanity.cpp`'s own Group A uses for allocation-COUNTING; this one
//     additionally INJECTS a failure at an exact, calibrated size (Group 2's own STAGE 2/2)
//     rather than only counting. Delegates to std::malloc/std::free, same as that file's own
//     override, for the identical reason (changes nothing observable about real allocation
//     behaviour outside of what this file itself arms).
// PT: Override global de operator new/delete -- a mesma técnica padrão que o próprio Grupo A de
//     image_decode_hardening_sanity.cpp usa pra CONTAR alocação; este aqui adicionalmente INJETA
//     uma falha num tamanho exato, calibrado (o próprio ESTÁGIO 2/2 do Grupo 2) em vez de só
//     contar. Delega pra std::malloc/std::free, igual ao próprio override daquele arquivo, pelo
//     motivo idêntico (não muda nada observável sobre o comportamento de alocação real fora do
//     que este arquivo mesmo arma).
void* operator new(std::size_t sz) {
  if (g_watching && g_seen_count < kMaxSeenSizes)
    g_seen_sizes[g_seen_count++] = sz;
  if (g_armed && sz == g_fail_size)
    throw std::bad_alloc(); // EN: THE injected failure. PT: A falha injetada.
  void* p = std::malloc(sz);
  if (p == nullptr)
    throw std::bad_alloc();
  return p;
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

// EN: Same VmSize reader as image_decode_hardening_sanity.cpp -- re-derived, not shared (these
//     are two independent test binaries, same "no shared header for this" convention that file's
//     own top comment already establishes for its own hostile-corpus builders).
// PT: Mesmo leitor de VmSize de image_decode_hardening_sanity.cpp -- re-derivado, não
//     compartilhado (são dois binários de teste independentes, mesma convenção "sem header
//     compartilhado pra isto" que o próprio comentário de topo daquele arquivo já estabelece pros
//     próprios construtores de corpus hostil).
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

} // namespace

using namespace glintfx;

int main() {
  // ===========================================================================================
  // EN: Group 1 -- ENC-NOTHROW oracle, WHOLE-CALL, forced-allocation-failure via
  //     fork()+RLIMIT_AS, calibrated in two stages (STAGE 1/2 CALIBRATE, STAGE 2/2 SQUEEZE), the
  //     SAME technique `image_decode_hardening_sanity.cpp`'s own Group B uses (see that file's
  //     own top comment for the full rationale and the mutation-testing story that proved a
  //     hand-derived fixed multiplier does NOT work reliably here). `kW`/`kH` chosen so the pixel
  //     buffer (and, for an uncompressed format like BMP, the encoded output too) is comfortably
  //     under `kMaxImageEncodeBytes` (64 MiB, `image_encode.cpp`) while still large enough that a
  //     genuine allocation failure is forceable.
  //     ⚠️ DECLARED SCOPE (adversarial review, W21): this group proves the WHOLE call degrades
  //     cleanly under real memory pressure, but structurally CANNOT discriminate
  //     `encode_image_file()`'s OWN `try`/`catch` specifically -- `encode_image_memory()` is
  //     itself `noexcept` with its own `try`, so the squeeze always exhausts memory INSIDE it
  //     first (its own peak dwarfs anything `encode_image_file()` adds on top), terminating at
  //     THAT boundary before ever reaching `encode_image_file()`'s frame. Proven by mutation: an
  //     earlier version of this file removed ONLY `encode_image_file()`'s own `try` and this
  //     group stayed GREEN -- see Group 2 below for the oracle that actually discriminates that
  //     specific `try`.
  // PT: Grupo 1 -- oráculo do ENC-NOTHROW, CHAMADA-INTEIRA, falha-de-alocação-forçada via
  //     fork()+RLIMIT_AS, calibrada em dois estágios (ESTÁGIO 1/2 CALIBRAR, ESTÁGIO 2/2 APERTAR),
  //     a MESMA técnica que o próprio Grupo B de `image_decode_hardening_sanity.cpp` usa (ver o
  //     próprio comentário de topo daquele arquivo pro racional completo e a história de teste de
  //     mutação que provou que um multiplicador fixo derivado à mão NÃO funciona de forma
  //     confiável aqui). `kW`/`kH` escolhidos pra que o buffer de pixel (e, pra um formato sem
  //     compressão como o BMP, a própria saída codificada também) fique confortavelmente abaixo
  //     de `kMaxImageEncodeBytes` (64 MiB, `image_encode.cpp`) mas ainda grande o bastante pra que
  //     uma falha de alocação genuína seja forçável.
  //     ⚠️ ESCOPO DECLARADO (review adversarial, W21): este grupo prova que a chamada INTEIRA
  //     degrada limpamente sob pressão de memória real, mas ESTRUTURALMENTE NÃO CONSEGUE
  //     discriminar o PRÓPRIO `try`/`catch` de `encode_image_file()` especificamente --
  //     `encode_image_memory()` em si é `noexcept` com o próprio `try`, então o aperto sempre
  //     esgota a memória DENTRO dela primeiro (o próprio pico dela ofusca qualquer coisa que
  //     `encode_image_file()` acrescente por cima), terminando NAQUELA fronteira antes de sequer
  //     alcançar o quadro de `encode_image_file()`. Provado por mutação: uma versão anterior deste
  //     arquivo removeu SÓ o próprio `try` de `encode_image_file()` e este grupo continuou VERDE
  //     -- ver o Grupo 2 abaixo pro oráculo que de fato discrimina aquele `try` específico.
  // ===========================================================================================
  {
    const int kW = 3000;
    const int kH = 2000;
    const std::size_t pixel_bytes = static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4u;
    check(pixel_bytes == 24u * 1000u * 1000u, "setup: expected pixel byte count is exactly 24,000,000");

    std::vector<unsigned char> pixels(pixel_bytes);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
      pixels[i + 0] = 30;
      pixels[i + 1] = 90;
      pixels[i + 2] = 180;
      pixels[i + 3] = 255;
    }

    // EN: A REAL, unconstrained call proves the fixture itself is sound before any memory-pressure
    //     experiment touches it -- if THIS fails, the calibration/squeeze children below would
    //     fail for the wrong reason (a bad fixture, not a forced allocation failure), which this
    //     repository's own "declare the limitation, don't test by luck" discipline requires ruling
    //     out first.
    // PT: Uma chamada REAL, sem restrição, prova que a própria fixture é sã antes de qualquer
    //     experimento de pressão de memória tocá-la -- se ISTO falhar, os filhos de calibração/
    //     aperto abaixo falhariam pelo motivo errado (uma fixture ruim, não uma falha de alocação
    //     forçada), o que a própria disciplina "declare a limitação, não teste por sorte" deste
    //     repositório exige descartar primeiro.
    {
      const EncodedImageBytes sane = encode_image_memory(ImageFormat::Bmp, kW, kH, pixels.data());
      check(sane.ok, "setup: unconstrained encode_image_memory(Bmp) on the real fixture succeeds");
    }

    char tmpl[] = "/var/tmp/glintfx_encode_hardening_XXXXXX";
    const int fd = mkstemp(tmpl);
    check(fd >= 0, "setup: mkstemp() scratch file created on /var/tmp");
    if (fd >= 0)
      close(fd); // EN: only need the NAME -- encode_image_file() opens it itself.
                 // PT: só precisa do NOME -- encode_image_file() abre ele mesmo.

    if (fd >= 0) {
      // =========================================================================================
      // EN: STAGE 1/2 -- CALIBRATE. See image_decode_hardening_sanity.cpp's own top comment for
      //     the full rationale (measure the REAL VmPeak-baseline delta of a full, successful,
      //     unconstrained call, via a pipe, rather than deriving a multiplier by hand).
      // PT: ESTÁGIO 1/2 -- CALIBRAR. Ver o próprio comentário de topo de
      //     image_decode_hardening_sanity.cpp pro racional completo (medir o delta REAL
      //     VmPeak-linha de base de uma chamada completa, bem-sucedida, sem restrição, via um
      //     pipe, em vez de derivar um multiplicador à mão).
      // =========================================================================================
      int cal_pipe[2] = {-1, -1};
      long cal_delta_bytes = -1;
      bool cal_ok = false;
      if (pipe(cal_pipe) == 0) {
        const pid_t cal_pid = fork();
        check(cal_pid >= 0, "calibration: fork() succeeded");
        if (cal_pid == 0) {
          close(cal_pipe[0]);
          const long b = read_vm_size_bytes();
          const bool ok = encode_image_file(tmpl, ImageFormat::Bmp, kW, kH, pixels.data());
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
          const long delta = (b >= 0 && peak_kb >= 0 && ok) ? (peak_kb * 1024 - b) : -1;
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
            "calibration: measured a real, positive peak-usage delta for an unconstrained "
            "encode_image_file(Bmp) call");

      if (!cal_ok) {
        std::fprintf(stderr,
                     "ENC-NOTHROW: SKIPPED -- calibration failed (pipe/fork/proc read "
                     "issue?) -- inconclusive, not a failure\n");
      } else {
        // =======================================================================================
        // EN: STAGE 2/2 -- SQUEEZE. baseline + cal_delta_bytes*3/4 -- same ratio measured to work
        //     reliably for the decode side's own Group B (image_decode_hardening_sanity.cpp).
        // PT: ESTÁGIO 2/2 -- APERTAR. baseline + cal_delta_bytes*3/4 -- mesma proporção medida
        //     pra funcionar de forma confiável no próprio Grupo B do lado decode
        //     (image_decode_hardening_sanity.cpp).
        // =======================================================================================
        const pid_t pid = fork();
        check(pid >= 0, "squeeze: fork() succeeded");

        if (pid == 0) {
          const long baseline = read_vm_size_bytes();
          if (baseline < 0)
            _exit(2); // EN: cannot measure -- inconclusive. PT: não dá pra medir -- inconclusivo.

          const long margin = cal_delta_bytes / 4;
          const rlim_t limit =
              static_cast<rlim_t>(baseline) + static_cast<rlim_t>(cal_delta_bytes - margin);
          struct rlimit rl;
          rl.rlim_cur = limit;
          rl.rlim_max = limit;
          if (setrlimit(RLIMIT_AS, &rl) != 0)
            _exit(3); // EN: could not clamp -- inconclusive. PT: não deu pra apertar -- inconclusivo.

          // EN: THE call under test -- encode_image_file() end-to-end (pixels in memory -> file on
          //     disk), exercising BOTH the encode_image_memory() delegation AND the ofstream
          //     write step this file's own top comment (image_encode.cpp) documents as its own
          //     additional risk. If ENC-NOTHROW regresses, std::bad_alloc propagates out of a
          //     noexcept function uncaught -> std::terminate() -> abort() -> SIGABRT, caught by
          //     the parent's WIFSIGNALED() check below.
          // PT: A chamada sob teste -- encode_image_file() ponta-a-ponta (pixels em memória ->
          //     arquivo em disco), exercitando TANTO a delegação a encode_image_memory() QUANTO o
          //     próprio passo de escrita ofstream que o próprio comentário de topo deste arquivo
          //     (image_encode.cpp) documenta como o próprio risco adicional. Se o ENC-NOTHROW
          //     regredir, std::bad_alloc se propaga pra fora de uma função noexcept não-capturada
          //     -> std::terminate() -> abort() -> SIGABRT, pego pela checagem WIFSIGNALED() do pai
          //     abaixo.
          const bool ok = encode_image_file(tmpl, ImageFormat::Bmp, kW, kH, pixels.data());
          _exit(ok ? 0 : 1); // 0 = unexpectedly succeeded despite the squeeze; 1 = false as expected.
        }

        int status = 0;
        const pid_t waited = waitpid(pid, &status, 0);
        check(waited == pid, "squeeze: waitpid() reaped the forked child");

        const bool signalled = WIFSIGNALED(status);
        char sig_msg[160];
        std::snprintf(sig_msg, sizeof(sig_msg),
                      "ENC-NOTHROW: child was NOT killed by a signal (uncaught std::bad_alloc "
                      "would SIGABRT) -- WIFSIGNALED=%d, signal=%d",
                      signalled ? 1 : 0, signalled ? WTERMSIG(status) : 0);
        check(!signalled, sig_msg);

        if (!signalled && WIFEXITED(status)) {
          const int code = WEXITSTATUS(status);
          if (code == 2) {
            std::fprintf(stderr,
                         "ENC-NOTHROW: SKIPPED -- child could not read /proc/self/status "
                         "VmSize -- inconclusive, not a failure\n");
          } else if (code == 3) {
            std::fprintf(stderr,
                         "ENC-NOTHROW: SKIPPED -- child's setrlimit(RLIMIT_AS) failed -- "
                         "inconclusive, not a failure\n");
          } else {
            check(code == 1,
                  "ENC-NOTHROW: encode_image_file() reported false under the forced squeeze "
                  "(exit 1 expected; 0 means the allocation unexpectedly fit -- widen the budget)");
          }
        }
      }

      std::remove(tmpl);
    }
  }

  // ===========================================================================================
  // EN: Group 2 -- ENC-NOTHROW oracle, OFSTREAM-SPECIFIC, size-targeted injection. Closes the
  //     gap Group 1 above declares: `encode_image_file()`'s own `try` exists specifically to
  //     catch a failure from `std::ofstream file(path, ...)`'s own internal streambuf allocation
  //     (`image_encode.cpp`'s own top comment) -- a REAL, measurable C++ allocation, but one no
  //     memory-BUDGET squeeze can isolate, because `encode_image_memory()`'s own noexcept
  //     boundary always absorbs the failure first. This group targets that ONE allocation
  //     DIRECTLY, by SIZE, not by budget: STAGE 1/2 CALIBRATES -- an UNARMED pass just WATCHES
  //     (via the global `operator new` override above) every allocation size a normal
  //     `encode_image_file()` call makes for a TINY 4x4 image, small enough that
  //     `encode_image_memory()`'s own allocations (a few dozen bytes for a 4x4 BMP header + pixel
  //     data) stay well under any plausible stream-buffer size -- so the LARGEST size observed is,
  //     with high confidence, the ofstream's own buffer, not anything `encode_image_memory()`
  //     touches. STAGE 2/2 SQUEEZES -- a forked child ARMS that exact, MEASURED (never hardcoded)
  //     size to throw `std::bad_alloc` on its next occurrence, then re-runs the IDENTICAL call.
  //     Measured on this toolchain (glibc/libstdc++, see the calibration's own stderr print
  //     below): the ofstream's own internal buffer is exactly 8192 bytes, allocated at
  //     construction -- but this group deliberately does NOT hardcode `8192`, precisely so it
  //     keeps discriminating correctly on a different libstdc++ version or standard library
  //     (libc++, MSVC's own STL) where the real size may differ; a wrong hardcoded guess would
  //     silently arm nothing and pass for the wrong reason, the exact class of lie this whole
  //     W21 wave exists to root out.
  // PT: Grupo 2 -- oráculo do ENC-NOTHROW, ESPECÍFICO DO OFSTREAM, injeção mirada por tamanho.
  //     Fecha a lacuna que o Grupo 1 acima declara: o próprio `try` de `encode_image_file()`
  //     existe especificamente pra capturar uma falha da própria alocação de streambuf interno de
  //     `std::ofstream file(path, ...)` (o próprio comentário de topo de `image_encode.cpp`) --
  //     uma alocação C++ REAL, mensurável, mas que nenhum aperto de ORÇAMENTO de memória consegue
  //     isolar, porque a própria fronteira noexcept de `encode_image_memory()` sempre absorve a
  //     falha primeiro. Este grupo mira aquela UMA alocação DIRETAMENTE, por TAMANHO, não por
  //     orçamento: o ESTÁGIO 1/2 CALIBRA -- uma passada DESARMADA só OBSERVA (via o override
  //     global de `operator new` acima) todo tamanho de alocação que uma chamada normal a
  //     `encode_image_file()` faz pra uma imagem 4x4 MINÚSCULA, pequena o bastante pra que as
  //     próprias alocações de `encode_image_memory()` (poucas dezenas de bytes pro cabeçalho BMP +
  //     dado de pixel de um 4x4) fiquem bem abaixo de qualquer tamanho plausível de buffer de
  //     stream -- então o MAIOR tamanho observado é, com alta confiança, o próprio buffer do
  //     ofstream, não nada que `encode_image_memory()` toque. O ESTÁGIO 2/2 APERTA -- um filho
  //     forkado ARMA aquele tamanho exato, MEDIDO (nunca fixado no código), pra lançar
  //     `std::bad_alloc` na próxima ocorrência dele, depois roda a mesma chamada IDÊNTICA de novo.
  //     Medido neste toolchain (glibc/libstdc++, ver o próprio print em stderr da calibração
  //     abaixo): o próprio buffer interno do ofstream é exatamente 8192 bytes, alocado na
  //     construção -- mas este grupo deliberadamente NÃO fixa `8192` no código, precisamente pra
  //     continuar discriminando corretamente numa versão diferente do libstdc++ ou biblioteca
  //     padrão diferente (libc++, a própria STL do MSVC) onde o tamanho real pode divergir; um
  //     chute fixado errado armaria silenciosamente nada e passaria pelo motivo errado, exatamente
  //     a classe de mentira que esta onda W21 inteira existe pra erradicar.
  // ===========================================================================================
  {
    const int kW2 = 4;
    const int kH2 = 4;
    std::vector<unsigned char> small_pixels(static_cast<std::size_t>(kW2) * static_cast<std::size_t>(kH2) * 4u);
    for (std::size_t i = 0; i < small_pixels.size(); i += 4) {
      small_pixels[i + 0] = 200;
      small_pixels[i + 1] = 100;
      small_pixels[i + 2] = 50;
      small_pixels[i + 3] = 255;
    }

    char tmpl2[] = "/var/tmp/glintfx_encode_hardening_ofs_XXXXXX";
    const int fd2 = mkstemp(tmpl2);
    check(fd2 >= 0, "Group 2 setup: mkstemp() scratch file created on /var/tmp");
    if (fd2 >= 0)
      close(fd2); // EN: only need the NAME. PT: só precisa do NOME.

    if (fd2 >= 0) {
      // EN: STAGE 1/2 -- CALIBRATE (unarmed watch).
      // PT: ESTÁGIO 1/2 -- CALIBRAR (observação desarmada).
      g_seen_count = 0;
      g_watching = true;
      const bool cal_ok = encode_image_file(tmpl2, ImageFormat::Bmp, kW2, kH2, small_pixels.data());
      g_watching = false;
      check(cal_ok, "Group 2 calibration: unconstrained encode_image_file(Bmp, 4x4) succeeds");

      std::size_t max_size = 0;
      for (int i = 0; i < g_seen_count; ++i)
        if (g_seen_sizes[i] > max_size)
          max_size = g_seen_sizes[i];
      check(max_size > 0, "Group 2 calibration: at least one allocation observed during the call");
      std::fprintf(stderr,
                   "Group 2: largest allocation observed during encode_image_file(4x4 BMP) was "
                   "%zu bytes (expected: the ofstream's own internal buffer)\n",
                   max_size);

      if (max_size > 0) {
        // EN: STAGE 2/2 -- SQUEEZE (armed size-targeted injection), forked for crash-isolation
        //     (same reason Group 1's own squeeze forks -- an uncaught bad_alloc here would abort
        //     the WHOLE test binary, not just this one check, if run in-process).
        // PT: ESTÁGIO 2/2 -- APERTAR (injeção armada mirada por tamanho), forkado por isolamento
        //     de crash (mesmo motivo do próprio aperto do Grupo 1 forkar -- um bad_alloc
        //     não-capturado aqui abortaria o BINÁRIO DE TESTE INTEIRO, não só esta checagem, se
        //     rodasse no mesmo processo).
        const pid_t pid = fork();
        check(pid >= 0, "Group 2 squeeze: fork() succeeded");

        if (pid == 0) {
          g_fail_size = max_size;
          g_armed = true;
          const bool ok = encode_image_file(tmpl2, ImageFormat::Bmp, kW2, kH2, small_pixels.data());
          _exit(ok ? 0 : 1);
        }

        int status = 0;
        const pid_t waited = waitpid(pid, &status, 0);
        check(waited == pid, "Group 2 squeeze: waitpid() reaped the forked child");

        const bool signalled = WIFSIGNALED(status);
        char sig_msg[220];
        std::snprintf(sig_msg, sizeof(sig_msg),
                      "Group 2 (ENC-NOTHROW, ofstream-specific): child was NOT killed by a signal "
                      "(uncaught std::bad_alloc from encode_image_file()'s own try would SIGABRT) "
                      "-- WIFSIGNALED=%d, signal=%d",
                      signalled ? 1 : 0, signalled ? WTERMSIG(status) : 0);
        check(!signalled, sig_msg);

        if (!signalled && WIFEXITED(status)) {
          check(WEXITSTATUS(status) == 1,
                "Group 2: encode_image_file() reported false when the ofstream's own internal "
                "buffer allocation was forced to fail (exit 1 expected; 0 means the injected "
                "size did not actually hit the ofstream, widen the calibration)");
        }
      }

      std::remove(tmpl2);
    }
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "image_encode_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("image_encode_hardening_sanity: PASS");
  return 0;
}
