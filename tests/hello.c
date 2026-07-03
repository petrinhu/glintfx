// SPDX-License-Identifier: MPL-2.0
// EN: The B6 gate program. Proves sys_write end to end: writes the literal "hello, world\n"
//     (13 bytes) to stdout (fd 1) via the raw `write` syscall, then exits 0 explicitly via
//     sys_exit -- no libc, no `printf`, no `strlen` (D2, not built yet). The byte count is
//     computed at compile time with `sizeof(literal) - 1`: `sizeof` of a string literal
//     includes the trailing '\0' the compiler appends, so subtracting 1 yields the exact
//     number of bytes we want written (13), with zero runtime cost and zero libc dependency.
// PT: O programa-gate da B6. Prova o sys_write ponta a ponta: escreve o literal
//     "hello, world\n" (13 bytes) no stdout (fd 1) via syscall `write` crua, depois sai com
//     codigo 0 explicitamente via sys_exit -- sem libc, sem `printf`, sem `strlen` (D2, ainda
//     nao construida). A contagem de bytes e' calculada em tempo de compilacao com
//     `sizeof(literal) - 1`: o `sizeof` de um literal de string inclui o '\0' final que o
//     compilador acrescenta, entao subtrair 1 da' o numero exato de bytes que queremos
//     escrever (13), com custo zero em runtime e zero dependencia de libc.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_write.h"
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    static const char msg[] = "hello, world\n";
    sys_write(1, msg, sizeof(msg) - 1);

    sys_exit(0);
    return 0; // unreachable -- sys_exit() above never returns
}
