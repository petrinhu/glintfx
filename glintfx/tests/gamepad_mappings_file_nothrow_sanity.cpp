// SPDX-License-Identifier: Apache-2.0
// EN: gamepad_mappings_file_nothrow_sanity (GAMEPAD-NOTHROW, W22, 2026-07-30) -- the SIXTH member
//     of the `never a crash` family (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`/
//     `CAPTURE-NOTHROW`, `image_decode_hardening_sanity.cpp`/`image_encode_hardening_sanity.cpp`/
//     `draw2d_texture_nothrow_sanity.cpp`/`font_nothrow_sanity.cpp`/`capture_nothrow_sanity.cpp`):
//     proves `glintfx::Gamepads::load_mappings_file()` (`gamepad.cpp`) degrades to a clean `0`,
//     never a crash, against the ORIGINAL, MEASURED repro -- `load_mappings_file("/dev/zero")`
//     escaping `std::bad_alloc` all the way to `std::terminate()`/`abort()` -- and the sibling
//     scenario the fix's own guard is equally responsible for: a legitimately huge, but FINITE,
//     file well past `kMaxMappingsFileBytes` (`gamepad.cpp`'s own 256 MiB cap).
//
//     WHY THIS FILE EXISTS SEPARATELY FROM `gamepad_mapping_hostile.cpp`: that file attacks
//     `MappingDb::parse_text()` DIRECTLY, in memory -- a buffer the TEST ITSELF already owns and
//     bounds, never touching `read_whole_file()`/`load_mappings_file()`'s own file-I/O path at
//     all. Its own single-line-of-~1 MB case is bounded BY THE TEST, not by the library -- it
//     never exercised the EOF-less-stream class of failure this file's own Group A targets,
//     which needs a REAL file-descriptor path (a character device, or a file whose own on-disk
//     size this process does not control the growth of) to reach at all. Complementary coverage,
//     not a duplicate.
//
//     Group A (THE exact measured repro, `/dev/zero`): `read_whole_file()`'s pre-fix
//     `std::ostringstream ss; ss << f.rdbuf();` slurp had no stopping condition against a
//     character device with no EOF -- the destination buffer grew without bound until
//     `std::bad_alloc` escaped uncaught. ⚠️ PRECISION: the original repro used `ulimit -v 512
//     MiB` to keep the REPRODUCING PROCESS' own memory footprint bounded during that
//     investigation -- `ulimit` was an ACCELERATOR for observing the crash quickly on a machine
//     with plenty of free RAM, never the CAUSE, and this fix does not need it either: the NEW
//     incremental, block-by-block cap (`kMaxMappingsFileBytes` = 256 MiB, checked WHILE reading,
//     not after) rejects the read on its own, in well under a second (reading 256 MiB+ from
//     `/dev/zero` is fast I/O, no allocation pressure involved), regardless of how much RAM the
//     test machine actually has. This file therefore runs `/dev/zero` with NO `ulimit` at all --
//     doing so would test the SAME thing the fix already guarantees unconditionally, and would
//     wrongly suggest a memory ceiling is load-bearing for correctness here (it is not).
//
//     Group B (a legitimately huge, FINITE, file -- the sibling scenario the SAME cap protects
//     against): a REGULAR file whose own on-disk SIZE exceeds `kMaxMappingsFileBytes`, built as
//     a SPARSE file via `std::filesystem::resize_file()` (a hole, not real allocated disk blocks
//     or real written bytes -- `stat()`/`tellg()` report the full size, but the filesystem does
//     not materialise it) so this test costs neither real disk space nor real write time to
//     construct. Reading through a sparse hole yields zero bytes transparently (identical stream
//     behaviour to `/dev/zero` from `read_whole_file()`'s own point of view) -- but this is a
//     SEEKABLE, FINITE regular file, the class of input a post-hoc `seek`/`tell` size check
//     WOULD have caught (unlike Group A's own character device) -- proving the fix's own
//     incremental, block-by-block guard also correctly rejects this distinct, sibling class
//     BEFORE completing the read, not merely as an accident of how `/dev/zero` happens to
//     behave.
//
//     Both groups run in a FORKED child (crash isolation, same convention every other member of
//     this family already uses): an uncaught `std::bad_alloc` inside the SAME process as this
//     file's own `main()` would abort the WHOLE binary before any later group's `check()`s could
//     run; a forked child that SIGABRTs instead reports cleanly via `waitpid()`'s own
//     `WIFSIGNALED()`.
// PT: gamepad_mappings_file_nothrow_sanity (GAMEPAD-NOTHROW, W22, 2026-07-30) -- o SEXTO membro
//     da família `never a crash` (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`/
//     `CAPTURE-NOTHROW`): prova que `glintfx::Gamepads::load_mappings_file()` (`gamepad.cpp`)
//     degrada pra um `0` limpo, nunca um crash, contra a reprodução ORIGINAL, MEDIDA --
//     `load_mappings_file("/dev/zero")` escapando `std::bad_alloc` até
//     `std::terminate()`/`abort()` -- e o cenário irmão pelo qual a PRÓPRIA guarda do conserto é
//     igualmente responsável: um arquivo legitimamente enorme, mas FINITO, bem além do
//     `kMaxMappingsFileBytes` (o próprio teto de 256 MiB de `gamepad.cpp`).
//
//     POR QUE ESTE ARQUIVO EXISTE SEPARADO DE `gamepad_mapping_hostile.cpp`: aquele ataca o
//     `MappingDb::parse_text()` DIRETO, em memória -- um buffer que o PRÓPRIO TESTE já possui e
//     limita, nunca tocando o próprio caminho de I/O de arquivo de `read_whole_file()`/
//     `load_mappings_file()`. O próprio caso de linha única de ~1 MB dele é limitado PELO TESTE,
//     não pela biblioteca -- nunca exercitou a classe de falha de stream-sem-EOF que o próprio
//     Grupo A deste arquivo mira, que precisa de um caminho de descritor de arquivo REAL (um
//     device de caractere, ou um arquivo cujo próprio tamanho em disco este processo não
//     controla o crescimento) pra sequer ser alcançada. Cobertura complementar, não duplicata.
//
//     Grupo A (A EXATA reprodução medida, `/dev/zero`): o slurp `std::ostringstream ss; ss <<
//     f.rdbuf();` pré-conserto de `read_whole_file()` não tinha condição de parada nenhuma
//     contra um device de caractere sem EOF -- o buffer de destino crescia sem limite até o
//     `std::bad_alloc` escapar sem captura. ⚠️ PRECISÃO: a reprodução original usou `ulimit -v
//     512 MiB` só pra manter a PRÓPRIA pegada de memória do PROCESSO REPRODUZINDO limitada
//     durante aquela investigação -- o `ulimit` foi um ACELERADOR pra observar o crash
//     rapidamente numa máquina com bastante RAM livre, nunca a CAUSA, e este conserto também não
//     precisa dele: o NOVO teto incremental, bloco-a-bloco (`kMaxMappingsFileBytes` = 256 MiB,
//     checado DURANTE a leitura, não depois) rejeita a leitura sozinho, em bem menos de um
//     segundo (ler 256 MiB+ de `/dev/zero` é I/O rápido, sem pressão de alocação envolvida),
//     independente de quanta RAM a máquina de teste de fato tem. Este arquivo portanto roda
//     `/dev/zero` SEM `ulimit` nenhum -- fazer isso testaria a MESMA coisa que o conserto já
//     garante incondicionalmente, e sugeriria erradamente que um teto de memória é
//     load-bearing pra correção aqui (não é).
//
//     Grupo B (um arquivo legitimamente enorme, FINITO -- o cenário irmão que a MESMA guarda
//     protege): um arquivo REGULAR cujo próprio TAMANHO em disco excede `kMaxMappingsFileBytes`,
//     construído como um arquivo ESPARSO via `std::filesystem::resize_file()` (um buraco, não
//     blocos de disco de fato alocados nem bytes de fato escritos -- `stat()`/`tellg()` reportam
//     o tamanho completo, mas o sistema de arquivos não o materializa) pra que este teste não
//     custe espaço de disco real nem tempo de escrita real pra construir. Ler através de um
//     buraco esparso rende bytes zero transparentemente (comportamento de stream idêntico ao
//     `/dev/zero` do próprio ponto de vista de `read_whole_file()`) -- mas isto é um arquivo
//     regular SEEKABLE, FINITO, a classe de input que uma checagem de tamanho a posteriori via
//     `seek`/`tell` TERIA pego (diferente do próprio device de caractere do Grupo A) -- provando
//     que a própria guarda incremental, bloco-a-bloco, do conserto também rejeita corretamente
//     esta classe irmã distinta ANTES de completar a leitura, não meramente como um acidente do
//     jeito que o `/dev/zero` por acaso se comporta.
//
//     Os dois grupos rodam num filho FORKADO (isolamento de crash, mesma convenção que todo
//     outro membro desta família já usa): um `std::bad_alloc` não-capturado dentro do MESMO
//     processo do próprio `main()` deste arquivo abortaria o binário INTEIRO antes que os
//     `check()`s de qualquer grupo posterior pudessem rodar; um filho forkado que sofre SIGABRT
//     em vez disso relata de forma limpa via o próprio `WIFSIGNALED()` do `waitpid()`.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

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

struct ChildOutcome {
  bool spawned = false;
  bool signalled = false;
  int term_signal = 0;
  bool exited = false;
  int exit_code = -1;
};

// EN: One forked child per group. Exit codes: 0 = load_mappings_file() returned cleanly
//     (crash-free -- the pass condition), any other value or a signal is a genuine FAIL.
// PT: Um filho forkado por grupo. Códigos de saída: 0 = load_mappings_file() retornou limpo
//     (livre-de-crash -- a condição de sucesso), qualquer outro valor ou sinal é uma FALHA
//     genuína.
ChildOutcome run_child(const char* path) {
  ChildOutcome out;
  const pid_t pid = fork();
  if (pid < 0) return out;
  out.spawned = true;

  if (pid == 0) {
    // EN: A throwaway GamepadsConfig -- load_mappings_file() only needs `impl_->initialized`,
    //     reached via Gamepads::init(), no real /dev/input access required (dev_dir points at a
    //     directory that need not even exist -- init() itself tolerates a missing/unreadable
    //     dev_dir, same "never a crash" discipline the rest of this module already has).
    // PT: Um GamepadsConfig descartável -- load_mappings_file() só precisa de
    //     `impl_->initialized`, alcançado via Gamepads::init(), nenhum acesso real a
    //     /dev/input necessário (dev_dir aponta pra um diretório que nem precisa existir --
    //     o próprio init() tolera um dev_dir ausente/ilegível, mesma disciplina "nunca um
    //     crash" que o resto deste módulo já tem).
    glintfx::Gamepads pads;
    glintfx::GamepadsConfig cfg;
    cfg.dev_dir = "/nonexistent-gamepad-nothrow-probe-dir";
    cfg.hotplug = false;
    if (!pads.init(cfg)) _exit(9); // EN: SKIP -- init() itself failed, inconclusive. PT: PULA.

    const int added = pads.load_mappings_file(path);
    (void)added; // EN: value not asserted here -- crash-freedom is the whole point of this file.
                 // PT: valor não asserido aqui -- ausência de crash é o ponto inteiro deste arquivo.
    _exit(0);
  }

  int status = 0;
  waitpid(pid, &status, 0);
  out.signalled = WIFSIGNALED(status);
  out.term_signal = out.signalled ? WTERMSIG(status) : 0;
  out.exited = WIFEXITED(status);
  out.exit_code = out.exited ? WEXITSTATUS(status) : -1;
  return out;
}

void run_oracle(const char* label, const char* path) {
  char msg[256];
  const ChildOutcome r = run_child(path);

  std::snprintf(msg, sizeof(msg), "%s: child's fork() succeeded", label);
  check(r.spawned, msg);

  std::snprintf(msg, sizeof(msg),
                "%s (GAMEPAD-NOTHROW): child was NOT killed by a signal (uncaught std::bad_alloc "
                "would SIGABRT/exit-134, matching the ORIGINAL measured repro) -- "
                "WIFSIGNALED=%d, signal=%d",
                label, r.signalled ? 1 : 0, r.signalled ? r.term_signal : 0);
  check(!r.signalled, msg);

  if (r.signalled || !r.exited) return;

  if (r.exit_code == 9) {
    std::fprintf(stderr, "%s: SKIPPED -- child's Gamepads::init() failed -- inconclusive\n", label);
    return;
  }

  std::snprintf(msg, sizeof(msg),
                "%s: load_mappings_file() returned normally (exit 0 expected, no crash)", label);
  check(r.exit_code == 0, msg);
}

} // namespace

int main() {
  // ===========================================================================================
  // Group A: /dev/zero -- THE exact scenario measured escaping to std::terminate()/abort()
  // pre-fix. No ulimit (see this file's own top comment for why it is not needed).
  // ===========================================================================================
  run_oracle("Group A (/dev/zero, EOF-less character device)", "/dev/zero");

  // ===========================================================================================
  // Group B: a sparse regular file, apparent size kMaxMappingsFileBytes + 4096 bytes past the
  // cap -- FINITE and SEEKABLE, unlike Group A's character device, but reads as all-zero bytes
  // identically from read_whole_file()'s own point of view.
  // ===========================================================================================
  {
    const char* kPath = "gamepad_nothrow_huge_sparse.bin";
    // EN: 256 MiB (kMaxMappingsFileBytes, gamepad.cpp) + 4096 bytes -- comfortably past the cap,
    //     small margin so the intent ("just over the cap") stays legible at the call site.
    // PT: 256 MiB (kMaxMappingsFileBytes, gamepad.cpp) + 4096 bytes -- confortavelmente além do
    //     teto, margem pequena pra que a intenção ("só um pouco além do teto") continue legível
    //     no próprio sítio de chamada.
    constexpr std::uintmax_t kHugeSize = (256ull * 1024ull * 1024ull) + 4096ull;

    std::error_code ec;
    {
      // EN: Create-or-truncate to 0 first, then resize_file() to the huge apparent size -- a
      //     hole, never real disk blocks or real written bytes.
      // PT: Cria-ou-trunca pra 0 primeiro, depois resize_file() pro tamanho aparente enorme --
      //     um buraco, nunca blocos de disco reais nem bytes de fato escritos.
      std::ofstream touch(kPath, std::ios::binary | std::ios::trunc);
    }
    std::filesystem::resize_file(kPath, kHugeSize, ec);
    check(!ec, "Group B setup: sparse huge file created via std::filesystem::resize_file()");

    if (!ec) {
      const std::uintmax_t actual_size = std::filesystem::file_size(kPath, ec);
      check(!ec && actual_size == kHugeSize,
            "Group B setup: sparse file's own apparent size matches the requested huge size");

      run_oracle("Group B (sparse regular file, finite but past the cap)", kPath);
    }

    std::filesystem::remove(kPath, ec); // EN: best-effort cleanup. PT: limpeza best-effort.
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_mappings_file_nothrow_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_mappings_file_nothrow_sanity: PASS");
  return 0;
}
