// SPDX-License-Identifier: MPL-2.0
// EN: The B5 gate program. Proves the FULL pipeline end to end: clang(freestanding) +
//     nasm(elf64) + ld(nostdlib) -> ELF64 static no-PIE -> kernel exec -> _start (B4) ->
//     main() -> sys_exit(42) (B5) -> raw `exit` syscall -> process terminates with code 42.
//     Calls sys_exit(42) EXPLICITLY (not `return 42;`): this also indirectly exercises
//     _start's fallback exit path (B4) -- if sys_exit() were somehow broken/a no-op, main()
//     would fall through to `return 0;` below, and _start's fallback would then exit with
//     code 0 instead of 42, making the test fail LOUDLY on the wrong code rather than
//     silently "passing by accident".
// PT: O programa-gate da B5. Prova o pipeline INTEIRO ponta a ponta: clang(freestanding) +
//     nasm(elf64) + ld(nostdlib) -> ELF64 estatico nao-PIE -> exec do kernel -> _start (B4)
//     -> main() -> sys_exit(42) (B5) -> syscall `exit` crua -> processo termina com codigo
//     42. Chama sys_exit(42) EXPLICITAMENTE (nao `return 42;`): isso tambem exercita
//     indiretamente o caminho de fallback do _start (B4) -- se sys_exit() estivesse
//     quebrado/fosse no-op, main() cairia no `return 0;` abaixo, e o fallback do _start
//     sairia entao com codigo 0 em vez de 42, fazendo o teste falhar ALTO no codigo errado
//     em vez de "passar por acidente" silenciosamente.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;
    sys_exit(42);
    return 0; // unreachable -- sys_exit() above never returns
}
