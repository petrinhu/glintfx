// SPDX-License-Identifier: MPL-2.0
// EN: sys_read -- see include/sys_read.h for the contract. Implemented as a 1-line wrapper
//     over syscall3(SYS_read, fd, buf, count) (B3/ADR-0001): all the register-shuffling logic
//     lives once, in syscall.asm -- this file adds only the name, the constant, and the two
//     pointer/size-to-`long` casts the raw syscall ABI (all-`long` args) requires.
// PT: sys_read -- ver include/sys_read.h pro contrato. Implementado como um wrapper de 1
//     linha sobre syscall3(SYS_read, fd, buf, count) (B3/ADR-0001): toda a logica de
//     reorganizacao de registrador mora uma unica vez, em syscall.asm -- este arquivo so
//     acrescenta o nome, a constante, e os dois casts ponteiro/tamanho-pra-`long` que a ABI
//     crua da syscall (argumentos todos `long`) exige.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_read.h"
#include "syscall.h"
#include "syscall_nums.h"

// EN: TST-STATIC (TODO.md, W11): cppcheck flags `buf` as "can be declared as pointer to const"
//     (constParameterPointer). False positive from this function's own C statements (it never
//     dereferences `buf` itself -- it only forwards the pointer to the raw `read` syscall,
//     src/syscall.asm), but the real contract is the opposite of const: the KERNEL writes INTO
//     `buf` during the syscall (see include/sys_read.h's doc-comment, "the kernel writes INTO
//     this buffer"). `const void*` here would be actively misleading about who mutates the
//     memory, on top of desyncing from the public `void*` declaration in sys_read.h. Suppressed
//     cirurgicamente, not the whole check.
// PT: TST-STATIC (TODO.md, W11): o cppcheck aponta `buf` como "pode ser declarado como
//     ponteiro pra const" (constParameterPointer). Falso positivo a partir das instruções C
//     desta função (ela mesma nunca desreferencia `buf` -- só repassa o ponteiro pra syscall
//     `read` crua, src/syscall.asm), mas o contrato real é o oposto de const: o KERNEL escreve
//     DENTRO de `buf` durante a syscall (ver o doc-comment de include/sys_read.h, "o kernel
//     escreve DENTRO deste buffer"). `const void*` aqui seria ativamente enganoso sobre quem
//     muta a memória, além de dessincronizar da declaração pública `void*` em sys_read.h.
//     Suprimido cirurgicamente, não a checagem inteira.
// cppcheck-suppress constParameterPointer
ssize_t sys_read(int fd, void* buf, size_t count) {
    return (ssize_t)syscall3(SYS_read, fd, (long)buf, (long)count);
}
