// SPDX-License-Identifier: MPL-2.0
// EN: SOV-LIBCORE (ADR-0009, FT-F0) -- the FIRST program in this repository that actually
//     exercises `build/libcore.a` as a hosted C++ consumer would (a future glintfx build is the
//     real target; this is the minimal proof). Built by `make libcore-test` (see the Makefile's
//     `libcore-test`/`$(BIN)/libcore_consumer` targets) with ORDINARY hosted flags (`$(CXXFLAGS)`
//     -- no `-ffreestanding`, no `-nostdlib`) via `$(CXX)` (clang++), linking against real
//     `libstdc++`/glibc AND `build/libcore.a` in the SAME binary -- this is deliberately NOT a
//     freestanding gate program (it lives in tests/ but is `.cpp`, so the Makefile's
//     `PROGRAM_SRCS := $(wildcard tests/*.c)` wildcard does NOT pick it up; it is built/run only
//     via the dedicated `libcore-test` target, never as part of `make build`/`make test`).
//
//     WHAT THIS PROVES (ADR-0009's 3 remaining gates, `core-v0.1.0` + the ADR itself already
//     closed gates 1/pilot's own prerequisites):
//     1. CORRECT USAGE round-trips cleanly through `glx_malloc`/`glx_realloc`/`glx_memcpy`/
//        `glx_memset`/`glx_free` -- ordinary allocate/write/read/grow/shrink/free, same shape
//        as tests/test_alloc.c's freestanding suite, just from a hosted TU.
//     2. TWO DISTINCT HEAPS coexist in the SAME process without interference: a pointer from
//        real `::malloc` (glibc) and a pointer from `glx_malloc` (this project's own allocator,
//        `mmap`-backed arenas, src/alloc.c) are written/read/freed independently, each through
//        its OWN matching free function, proving the archive did not silently hijack glibc's
//        allocator (see tools/check_libcore_symbols.sh, run right after this program by `make
//        libcore-test`, for the symbol-level proof backing this).
//     3. THE OWNERSHIP-CROSSING MISUSE ADR-0009 gate 3 exists specifically to guard against
//        FAILS LOUDLY, not silently, in BOTH directions -- proved LIVE, not merely asserted,
//        via two forked child processes (a crash/abort in either direction must not take this
//        whole test binary down with it, hence `fork()` + `waitpid()` rather than calling the
//        misuse directly in this process):
//        (a) `glx_free()` on a pointer from glibc's `::malloc` -- src/core_api.c's
//            `alloc_owns_ptr` boundary check (include/alloc_internal.h) detects this is not one
//            of THIS allocator's own arenas and terminates the child via `sys_exit(112)`
//            (`GLX_FREE_FOREIGN_PTR_EXIT_CODE`, src/core_api.c) -- asserted here by exact exit
//            code, our OWN defense-in-depth.
//        (b) `::free()` (glibc's) on a pointer from `glx_malloc` -- this project builds NO
//            defense on this side (glibc's allocator is not ours to instrument); the assertion
//            here is the weaker, honest one ADR-0009 itself anticipates ("ASan will not catch
//            [this] across the boundary" is exactly why gate 3 demands a DEDICATED test, not a
//            sanitizer run): glibc's OWN internal chunk-consistency checks are relied on to
//            reject an unrecognised chunk header loudly (`SIGABRT`/non-zero exit), which this
//            test asserts DID happen -- if it ever silently "succeeded" instead, that is exactly
//            the "silent heap corruption" ADR-0009 warns about, and this test would FAIL loudly
//            to surface it, never pass silently.
//        (c) `glx_free()` on a MISALIGNED offset into a block this allocator legitimately owns
//            (`glx_free(p + 8)`) -- SECURITY-ENGINEER FINDING (post-decb2fb review, IMPORTANT):
//            `alloc_owns_ptr`'s original arena-bounds-only check let this slip past gate 3 and
//            SIGSEGV inside `free()`'s header arithmetic instead of dying via the documented
//            `sys_exit(112)`; the fix (an alignment check, include/alloc_internal.h/src/alloc.c)
//            is proven here the same way as (a): exact exit code, forked, live-executed.
// PT: SOV-LIBCORE (ADR-0009, FT-F0) -- o PRIMEIRO programa deste repositório que de fato
//     exercita a `build/libcore.a` como um consumidor C++ hosted exerceria (um futuro build do
//     glintfx é o alvo real; isto é a prova mínima). Buildado por `make libcore-test` (ver os
//     alvos `libcore-test`/`$(BIN)/libcore_consumer` do Makefile) com flags hosted COMUNS
//     (`$(CXXFLAGS)` -- sem `-ffreestanding`, sem `-nostdlib`) via `$(CXX)` (clang++), linkando
//     contra `libstdc++`/glibc de verdade E `build/libcore.a` no MESMO binário -- isto é
//     deliberadamente NÃO um programa-gate freestanding (mora em tests/ mas é `.cpp`, então o
//     wildcard `PROGRAM_SRCS := $(wildcard tests/*.c)` do Makefile NÃO pega ele; é
//     buildado/rodado só via o alvo dedicado `libcore-test`, nunca como parte de `make
//     build`/`make test`).
//
//     O QUE ISTO PROVA (os 3 gates restantes da ADR-0009 -- `core-v0.1.0` + a própria ADR já
//     fecharam o gate 1/os pré-requisitos do próprio piloto):
//     1. USO CORRETO faz round-trip limpo por `glx_malloc`/`glx_realloc`/`glx_memcpy`/
//        `glx_memset`/`glx_free` -- alocar/escrever/ler/crescer/encolher/liberar comum, mesma
//        forma da suíte freestanding do tests/test_alloc.c, só que a partir de uma TU hosted.
//     2. DOIS HEAPS DISTINTOS coexistem no MESMO processo sem interferência: um ponteiro do
//        `::malloc` de verdade (glibc) e um ponteiro do `glx_malloc` (o alocador PRÓPRIO deste
//        projeto, arenas lastreadas em `mmap`, src/alloc.c) são escritos/lidos/liberados
//        independentemente, cada um pela SUA função de free correspondente, provando que o
//        archive não sequestrou o alocador da glibc em silêncio (ver
//        tools/check_libcore_symbols.sh, rodado logo depois deste programa pelo `make
//        libcore-test`, pra prova em nível de símbolo que sustenta isso).
//     3. O MAU-USO DE CRUZAMENTO DE OWNERSHIP que o gate 3 da ADR-0009 existe especificamente
//        pra guardar contra FALHA ALTO, não em silêncio, nas DUAS direções -- provado AO VIVO,
//        não só afirmado, via dois processos filho via fork (um crash/abort em qualquer direção
//        não pode derrubar este binário de teste inteiro junto, daí `fork()` + `waitpid()` em
//        vez de chamar o mau-uso direto neste processo):
//        (a) `glx_free()` num ponteiro do `::malloc` da glibc -- a checagem de fronteira
//            `alloc_owns_ptr` do src/core_api.c (include/alloc_internal.h) detecta que não é
//            uma das arenas PRÓPRIAS deste alocador e termina o filho via `sys_exit(112)`
//            (`GLX_FREE_FOREIGN_PTR_EXIT_CODE`, src/core_api.c) -- verificado aqui pelo exit
//            code exato, nossa PRÓPRIA defesa em profundidade.
//        (b) `::free()` (da glibc) num ponteiro do `glx_malloc` -- este projeto não constrói
//            nenhuma defesa deste lado (o alocador da glibc não é nosso pra instrumentar); a
//            verificação aqui é a mais fraca, honesta, que a própria ADR-0009 antecipa ("o ASan
//            não pega [isso] através da fronteira" é exatamente por que o gate 3 exige um teste
//            DEDICADO, não uma corrida de sanitizer): as próprias checagens internas de
//            consistência de chunk da glibc são a que se confia pra rejeitar alto um header de
//            chunk não-reconhecido (`SIGABRT`/exit não-zero), que este teste verifica que DE
//            FATO aconteceu -- se algum dia "tivesse sucesso" em silêncio, seria exatamente a
//            "corrupção de heap silenciosa" que a ADR-0009 avisa, e este teste FALHARIA alto pra
//            revelar isso, nunca passaria em silêncio.
//        (c) `glx_free()` num offset DESALINHADO dentro de um bloco que este alocador possui
//            legitimamente (`glx_free(p + 8)`) -- ACHADO DO SECURITY-ENGINEER (revisão
//            pós-decb2fb, IMPORTANTE): a checagem original do `alloc_owns_ptr`, só-de-limites-de-
//            arena, deixava isso escapar do gate 3 e dar SIGSEGV dentro da aritmética de header
//            do `free()` em vez de morrer via o `sys_exit(112)` documentado; o fix (uma checagem
//            de alinhamento, include/alloc_internal.h/src/alloc.c) é provado aqui do mesmo jeito
//            que (a): exit code exato, via fork, executado ao vivo.
// Copyright (c) 2026 Petrus Silva Costa
#include "core/core.h"

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

// EN: Mirrors src/core_api.c's `GLX_FREE_FOREIGN_PTR_EXIT_CODE` -- NOT part of the public
//     include/core/core.h contract (deliberately: a hosted caller should never branch on a
//     specific misuse exit code, exactly like include/alloc.h's own double-free note says never
//     to branch on double-free's termination signal/code), duplicated here ONLY because this
//     test needs to assert the SPECIFIC value to prove the RIGHT defense fired (not merely
//     "something crashed").
// PT: Espelha o `GLX_FREE_FOREIGN_PTR_EXIT_CODE` do src/core_api.c -- NÃO faz parte do contrato
//     público do include/core/core.h (de propósito: quem chama hosted nunca deveria ramificar
//     num exit code específico de mau-uso, exatamente como a própria nota de double-free do
//     include/alloc.h diz pra nunca ramificar no sinal/código de terminação do double-free),
//     duplicado aqui SÓ porque este teste precisa verificar o valor ESPECÍFICO pra provar que a
//     defesa CERTA disparou (não só "algo crashou").
constexpr int kGlxFreeForeignPtrExitCode = 112;

int g_failures = 0;

void check(bool cond, const char* what) {
    if (cond) {
        std::printf("PASS: %s\n", what);
    } else {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

// EN: QW-NOCORE (INBOX drain, Onda 1) -- signal handler that turns a fatal `SIGABRT` into a
//     plain, non-signalled process exit. See `run_in_forked_child`'s own comment (right below)
//     for the full rationale of WHY this is needed on top of `setrlimit(RLIMIT_CORE, ...)`
//     (short version: `RLIMIT_CORE` alone only suppresses the core FILE, not the
//     `systemd-coredump`/DrKonqi pipeline itself, which still fires on any UNCAUGHT fatal
//     signal regardless of the rlimit). `_exit(128 + sig)` (not `exit`): the conventional
//     "died from signal N" shell exit-code convention, `_exit` (not `std::exit`) because this
//     runs inside a signal handler in a forked child -- skipping atexit handlers/stdio flushing
//     here is correct, not a shortcut (the parent's own buffered stdout must never be touched by
//     the child, exactly the same reasoning `_exit(0)` at the end of `run_in_forked_child`
//     already uses). `extern "C"` linkage: `sigaction`'s `sa_handler` is a C function pointer
//     type; POSIX signal handlers are always installed with C linkage.
// PT: Handler de sinal do QW-NOCORE (esvaziamento da INBOX, Onda 1) que transforma um `SIGABRT`
//     fatal num exit de processo comum, sem sinal. Ver o comentário próprio do
//     `run_in_forked_child` (logo abaixo) pro racional completo do PORQUÊ disto ser necessário
//     em cima do `setrlimit(RLIMIT_CORE, ...)` (versão curta: o `RLIMIT_CORE` sozinho só
//     suprime o ARQUIVO de core, não o próprio pipeline `systemd-coredump`/DrKonqi, que ainda
//     dispara em qualquer sinal fatal NÃO-CAPTURADO independente do rlimit). `_exit(128 + sig)`
//     (não `exit`): a convenção comum de exit-code de shell "morreu do sinal N", `_exit` (não
//     `std::exit`) porque isto roda dentro de um handler de sinal num filho forkado -- pular
//     handlers de atexit/flush de stdio aqui é correto, não um atalho (o próprio stdout
//     bufferizado do PAI nunca pode ser tocado pelo filho, exatamente o mesmo raciocínio que o
//     `_exit(0)` no fim do `run_in_forked_child` já usa). Linkage `extern "C"`: o `sa_handler`
//     do `sigaction` é um tipo de ponteiro-de-função C; handlers de sinal POSIX são sempre
//     instalados com linkage C.
extern "C" void child_exit_on_abort(int sig) { _exit(128 + sig); }

// EN: Runs `fn` in a forked child, waits for it, and returns the raw `waitpid` status word for
//     the CALLER to interpret (different misuse directions expect different termination shapes
//     -- see file header). Isolates a deliberately-crashing/aborting scenario from this test
//     binary's own process.
// PT: Roda `fn` num filho via fork, espera ele, e retorna a palavra de status crua do
//     `waitpid` pra quem CHAMA interpretar (direções de mau-uso diferentes esperam formas de
//     terminação diferentes -- ver cabeçalho do arquivo). Isola um cenário que crasha/aborta de
//     propósito do próprio processo deste binário de teste.
template <typename Fn>
int run_in_forked_child(Fn fn) {
    pid_t pid = fork();
    if (pid < 0) {
        std::perror("fork");
        std::exit(97);
    }
    if (pid == 0) {
        // EN: QW-NOCORE (INBOX drain, Onda 1) -- TWO layers, applied to EVERY forked child (not
        //     scattered per call site), BEFORE running `fn`, so a scenario that deliberately
        //     aborts/crashes never wakes systemd-coredump/DrKonqi or writes a core file. Neither
        //     layer changes which signal is delivered, whether the child is terminated by a
        //     signal at all (for the OTHER, uncaught-signal case layer 1 alone would still leave
        //     that true), or the `waitpid` status word the PARENT inspects afterwards -- every
        //     assertion in cases 3a/3b/3c keeps checking EXACTLY the same contract, just without
        //     the disk/journal/DrKonqi side effect:
        //       1. `setrlimit(RLIMIT_CORE, {0, 0})` -- disables the kernel's core-FILE-writing
        //          step for this process. Necessary but NOT sufficient on its own (verified
        //          live, not assumed): with only this layer, `coredumpctl list` correctly showed
        //          `COREFILE: none`, but `journalctl` STILL showed a fresh `ANOM_ABEND`/SIGABRT
        //          audit entry AND a `drkonqi-coredump-processor`/`drkonqi-coredump-launcher`
        //          service pair being started -- because the kernel's `core_pattern` pipes EVERY
        //          fatal, uncaught signal to `systemd-coredump` regardless of `RLIMIT_CORE` (the
        //          rlimit only tells `systemd-coredump` whether to PERSIST the core, not whether
        //          to be invoked at all). Kept anyway as defense in depth for any FUTURE misuse
        //          scenario whose fatal signal this file does not explicitly catch (layer 2 below
        //          only targets the one signal actually raised by TODAY's scenarios, `SIGABRT`).
        //       2. `sigaction(SIGABRT, child_exit_on_abort, ...)` -- the layer that actually
        //          closes the gap layer 1 leaves open for case 3b (glibc's own chunk-consistency
        //          check calling `abort()` on `std::free()` of a `glx_malloc` pointer -- this
        //          file's own header, item 3(b); that abort is the EXPECTED, PASSING outcome of
        //          this test, not a real crash). A CAUGHT signal is, by definition, never an
        //          "uncaught fatal signal" from the kernel's point of view, so `core_pattern`'s
        //          pipe to `systemd-coredump` is never triggered for it at all (verified live,
        //          see child_exit_on_abort's own comment for what this converts the termination
        //          into) -- confirmed AFTER adding this layer: `coredumpctl list --since` shows
        //          NO new entry whatsoever (not even a `COREFILE: none` one) for a fresh `make
        //          libcore-test` run, and `journalctl` carries no `ANOM_ABEND`/DrKonqi lines for
        //          it either.
        // PT: QW-NOCORE (esvaziamento da INBOX, Onda 1) -- DUAS camadas, aplicadas a TODO filho
        //     forkado (não espalhadas por ponto de chamada), ANTES de rodar `fn`, pra que um
        //     cenário que aborta/crasha de propósito nunca acorde o systemd-coredump/DrKonqi nem
        //     escreva um arquivo de core. Nenhuma das duas camadas muda qual sinal é entregue, se
        //     o filho é terminado por sinal ou não (pro OUTRO caso, o de sinal-não-capturado, a
        //     camada 1 sozinha ainda deixaria isso verdadeiro), nem a palavra de status do
        //     `waitpid` que o PAI inspeciona depois -- toda asserção dos casos 3a/3b/3c continua
        //     checando EXATAMENTE o mesmo contrato, só sem o efeito colateral de
        //     disco/journal/DrKonqi:
        //       1. `setrlimit(RLIMIT_CORE, {0, 0})` -- desliga o passo do kernel de escrever o
        //          ARQUIVO de core pra este processo. Necessário mas NÃO suficiente sozinho
        //          (verificado ao vivo, não suposto): só com esta camada, o `coredumpctl list`
        //          mostrava corretamente `COREFILE: none`, mas o `journalctl` AINDA mostrava uma
        //          entrada de auditoria `ANOM_ABEND`/SIGABRT fresca E um par de serviços
        //          `drkonqi-coredump-processor`/`drkonqi-coredump-launcher` sendo iniciado --
        //          porque o `core_pattern` do kernel envia por pipe TODO sinal fatal
        //          não-capturado pro `systemd-coredump` independente do `RLIMIT_CORE` (o rlimit
        //          só diz ao `systemd-coredump` se deve PERSISTIR o core, não se deve ser
        //          invocado). Mantido mesmo assim como defesa-em-profundidade pra qualquer
        //          cenário FUTURO de mau-uso cujo sinal fatal este arquivo não capture
        //          explicitamente (a camada 2 abaixo só mira o único sinal de fato levantado
        //          pelos cenários de HOJE, `SIGABRT`).
        //       2. `sigaction(SIGABRT, child_exit_on_abort, ...)` -- a camada que de fato fecha a
        //          lacuna que a camada 1 deixa aberta pro caso 3b (a própria checagem de
        //          consistência de chunk da glibc chamando `abort()` no `std::free()` de um
        //          ponteiro do `glx_malloc` -- cabeçalho próprio deste arquivo, item 3(b); aquele
        //          abort é o resultado ESPERADO, de teste PASSANDO, não um crash de verdade). Um
        //          sinal CAPTURADO nunca é, por definição, um "sinal fatal não-capturado" do
        //          ponto de vista do kernel, então o pipe do `core_pattern` pro `systemd-coredump`
        //          nunca sequer dispara pra ele (verificado ao vivo, ver o comentário próprio do
        //          child_exit_on_abort pro que isto transforma a terminação) -- confirmado DEPOIS
        //          de acrescentar esta camada: `coredumpctl list --since` não mostra NENHUMA
        //          entrada nova (nem uma de `COREFILE: none`) pra uma rodada fresca de `make
        //          libcore-test`, e o `journalctl` não carrega linha `ANOM_ABEND`/DrKonqi pra ela
        //          tampouco.
        struct rlimit no_core = {0, 0};
        setrlimit(RLIMIT_CORE, &no_core);
        std::signal(SIGABRT, child_exit_on_abort);

        fn();
        _exit(0); // EN/PT: only reached if `fn` returned without crashing/exiting itself.
    }
    int status = 0;
    if (waitpid(pid, &status, 0) != pid) {
        std::perror("waitpid");
        std::exit(97);
    }
    return status;
}

} // namespace

int main() {
    // ---- 1. Correct usage: glx_malloc -> glx_memset -> glx_memcpy -> glx_realloc -> glx_free
    {
        char* buf = static_cast<char*>(glx_malloc(64));
        check(buf != nullptr, "glx_malloc(64) returns non-NULL");
        check(reinterpret_cast<std::uintptr_t>(buf) % 16 == 0,
              "glx_malloc(64) is 16-byte aligned");

        glx_memset(buf, 'A', 64);
        check(buf[0] == 'A' && buf[63] == 'A', "glx_memset wrote every requested byte");

        char src[] = "sov-libcore";
        glx_memcpy(buf, src, sizeof(src));
        check(std::memcmp(buf, src, sizeof(src)) == 0, "glx_memcpy copied bytes correctly");

        char* grown = static_cast<char*>(glx_realloc(buf, 256));
        check(grown != nullptr, "glx_realloc(ptr, 256) (grow) returns non-NULL");
        check(std::memcmp(grown, src, sizeof(src)) == 0,
              "glx_realloc(grow) preserved the original content");

        char* shrunk = static_cast<char*>(glx_realloc(grown, 8));
        check(shrunk != nullptr, "glx_realloc(ptr, 8) (shrink) returns non-NULL");
        check(std::memcmp(shrunk, src, 8) == 0,
              "glx_realloc(shrink) preserved the surviving prefix");

        glx_free(shrunk);
        check(true, "glx_free on a legitimately-owned pointer did not crash");
    }

    // ---- 2. Two distinct heaps coexist, each freed through its OWN matching function --------
    {
        void* glibc_ptr = std::malloc(128); // real glibc allocator, bare `::malloc`
        void* glx_ptr   = glx_malloc(128);  // this project's own allocator
        check(glibc_ptr != nullptr && glx_ptr != nullptr,
              "both a glibc pointer and a glx_ pointer allocate successfully in the same process");
        check(glibc_ptr != glx_ptr, "the two allocators never hand out the same address");

        std::memset(glibc_ptr, 0xAB, 128);
        glx_memset(glx_ptr, 0xCD, 128);
        check(static_cast<unsigned char*>(glibc_ptr)[0] == 0xAB &&
                  static_cast<unsigned char*>(glx_ptr)[0] == 0xCD,
              "writes through each pointer land in genuinely separate memory (no aliasing)");

        std::free(glibc_ptr); // glibc pointer -> glibc's own free() -- CORRECT pairing
        glx_free(glx_ptr);    // glx_ pointer  -> glx_free()          -- CORRECT pairing
        check(true, "each pointer freed through its own matching allocator did not crash");
    }

    // ---- 3a. Misuse: glx_free() on a FOREIGN (glibc) pointer -> must fail LOUDLY (our gate) --
    {
        int status = run_in_forked_child([] {
            void* foreign = std::malloc(32);
            glx_free(foreign); // ADR-0009 gate 3 misuse, direction 1
            // EN/PT: unreachable if the boundary check fired as designed.
        });
        bool exited_with_our_code =
            WIFEXITED(status) && WEXITSTATUS(status) == kGlxFreeForeignPtrExitCode;
        check(exited_with_our_code,
              "glx_free() on a glibc pointer terminates the process via our own boundary check "
              "(exit 112), not silent corruption");
    }

    // ---- 3b. Misuse: glibc's free() on a glx_ pointer -> must fail LOUDLY (glibc's own guard) -
    {
        int status = run_in_forked_child([] {
            void* ours = glx_malloc(32);
            std::free(ours); // ADR-0009 gate 3 misuse, direction 2 (glibc's chunk-header check)
            // EN/PT: if glibc's allocator ever silently accepts this, it reaches here and
            //        exits 0 below -- exactly the "silent corruption" failure mode this
            //        assertion exists to catch, not paper over.
            _exit(0);
        });
        bool glibc_rejected_it =
            WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0);
        check(glibc_rejected_it,
              "glibc's free() rejects a foreign (glx_) pointer loudly (abort/non-zero exit), "
              "never silently \"succeeding\"");
    }

    // ---- 3c. Misuse: glx_free() on a MISALIGNED offset into a LEGITIMATELY-owned block --------
    // EN: SECURITY-ENGINEER FINDING (post-decb2fb review, IMPORTANT, this case proves the fix):
    //     `alloc_owns_ptr` (include/alloc_internal.h, src/alloc.c) originally only checked that
    //     the computed header address fell within SOME arena's `[base, end)` bounds -- it did NOT
    //     check that the address was actually a real block header, so `glx_free(p + 8)` on a
    //     pointer `p` this allocator genuinely handed out would slip PAST the gate-3 boundary
    //     check and reach `free()`'s header-arithmetic/coalescing logic with garbage
    //     `size_and_flags`/`arena` fields -- undefined behaviour, reproduced as SIGSEGV, NOT the
    //     documented `sys_exit(112)`. The fix adds an alignment check (every real block header's
    //     offset from its arena's `base` is a multiple of 16 by construction); this case proves
    //     `glx_free(p + 8)` (offset 8, NOT a multiple of 16) now dies via the documented exit
    //     code, exactly like the direction-1 foreign-pointer misuse in case 3a above, never a
    //     signal.
    // PT: ACHADO DO SECURITY-ENGINEER (revisão pós-decb2fb, IMPORTANTE, este caso prova o fix): o
    //     `alloc_owns_ptr` (include/alloc_internal.h, src/alloc.c) originalmente só checava que o
    //     endereço de header computado caía dentro dos limites `[base, end)` de ALGUMA arena --
    //     NÃO checava que o endereço era de fato um header de bloco real, então
    //     `glx_free(p + 8)` num ponteiro `p` que este alocador de fato entregou escapava PELA
    //     checagem de fronteira do gate 3 e chegava à lógica de aritmética-de-header/
    //     coalescência do `free()` com campos `size_and_flags`/`arena` lixo -- comportamento
    //     indefinido, reproduzido como SIGSEGV, NÃO o `sys_exit(112)` documentado. O fix
    //     acrescenta uma checagem de alinhamento (o offset de todo header de bloco real a partir
    //     do `base` da arena dele é múltiplo de 16 por construção); este caso prova que
    //     `glx_free(p + 8)` (offset 8, NÃO múltiplo de 16) agora morre via o exit code
    //     documentado, exatamente como o mau-uso de ponteiro-estrangeiro da direção 1 no caso 3a
    //     acima, nunca um sinal.
    {
        int status = run_in_forked_child([] {
            char* p = static_cast<char*>(glx_malloc(64)); // legitimately owned by glx_
            glx_free(p + 8); // ADR-0009 gate 3 misuse: misaligned offset into an OWNED block
            // EN/PT: unreachable if the alignment check fired as designed.
        });
        bool exited_with_our_code =
            WIFEXITED(status) && WEXITSTATUS(status) == kGlxFreeForeignPtrExitCode;
        check(exited_with_our_code,
              "glx_free(p + 8) on a legitimately-owned-but-misaligned pointer terminates the "
              "process via our own boundary check (exit 112), not SIGSEGV");
    }

    if (g_failures == 0) {
        std::printf("libcore_consumer: ALL CHECKS PASSED\n");
        return 0;
    }
    std::printf("libcore_consumer: %d CHECK(S) FAILED\n", g_failures);
    return 1;
}
