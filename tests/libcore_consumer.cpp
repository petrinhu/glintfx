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
// Copyright (c) 2026 Petrus Silva Costa
#include "core/core.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

    if (g_failures == 0) {
        std::printf("libcore_consumer: ALL CHECKS PASSED\n");
        return 0;
    }
    std::printf("libcore_consumer: %d CHECK(S) FAILED\n", g_failures);
    return 1;
}
