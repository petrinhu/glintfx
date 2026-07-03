// SPDX-License-Identifier: MPL-2.0
// EN: The B7 gate program. Proves sys_read end to end: a minimal `cat` -- reads from stdin
//     (fd 0) into a local 256-byte buffer via sys_read, and echoes exactly the `n` bytes read
//     back to stdout (fd 1) via sys_write (B6), looping until EOF (sys_read returns 0). No
//     libc, no `getchar`/`fread`, no buffering beyond the one on-stack array. Three outcomes
//     per read() are handled explicitly, matching the raw kernel contract (ADR-0002): `n > 0`
//     echoes those `n` bytes (never the whole buffer -- the tail past `n` is uninitialized
//     stack garbage from a previous, larger read, or never written at all); `n == 0` is EOF,
//     breaks the loop, falls through to sys_exit(0); `n < 0` is a raw `-errno`, exits non-zero
//     immediately (the low byte of `-n` is reused as the exit code -- not POSIX errno
//     numbering by the shell, just a deterministic non-zero signal, sufficient since this
//     project has no libc `strerror` to decode it anyway).
// PT: O programa-gate da B7. Prova o sys_read ponta a ponta: um `cat` minimo -- le do stdin
//     (fd 0) para um buffer local de 256 bytes via sys_read, e ecoa exatamente os `n` bytes
//     lidos de volta no stdout (fd 1) via sys_write (B6), em loop ate EOF (sys_read retorna
//     0). Sem libc, sem `getchar`/`fread`, sem buffering alem do unico array na stack. Os tres
//     desfechos possiveis de cada read() sao tratados explicitamente, seguindo o contrato cru
//     do kernel (ADR-0002): `n > 0` ecoa esses `n` bytes (nunca o buffer inteiro -- a cauda
//     apos `n` e' lixo de stack nao-inicializado de uma leitura anterior maior, ou nunca
//     escrita); `n == 0' e' EOF, sai do loop, cai no sys_exit(0); `n < 0` e' um `-errno` cru,
//     sai com codigo nao-zero imediatamente (o byte baixo de `-n` e' reaproveitado como codigo
//     de saida -- nao e' a numeracao errno do POSIX interpretada pela shell, so' um sinal
//     nao-zero deterministico, suficiente ja que este projeto nao tem `strerror` da libc pra
//     decodificar mesmo assim).
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_read.h"
#include "sys_write.h"
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    char buf[256];

    for (;;) {
        ssize_t n = sys_read(0, buf, sizeof(buf));

        if (n > 0) {
            sys_write(1, buf, (size_t)n);
        } else if (n == 0) {
            break; // EOF
        } else {
            sys_exit((int)(-n) & 0xff); // error: raw -errno, non-zero exit
        }
    }

    sys_exit(0);
    return 0; // unreachable -- sys_exit() above never returns
}
