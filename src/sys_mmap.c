// SPDX-License-Identifier: MPL-2.0
// EN: sys_mmap -- see include/sys_mmap.h for the contract. Implemented as a 1-line wrapper
//     over syscall6(SYS_mmap, ...) (B3/ADR-0001): all the register-shuffling logic lives once,
//     in syscall.asm -- this file adds only the name, the constant, and the casts the raw
//     syscall ABI (all-`long` args) requires. `fd` and `off` are conventionally `-1`/`0` for
//     the anonymous mapping E1 uses (MAP_ANONYMOUS ignores them), but this wrapper is generic
//     over both -- it does not hardcode that assumption.
// PT: sys_mmap -- ver include/sys_mmap.h pro contrato. Implementado como um wrapper de 1 linha
//     sobre syscall6(SYS_mmap, ...) (B3/ADR-0001): toda a lógica de reorganização de
//     registrador mora uma única vez, em syscall.asm -- este arquivo só acrescenta o nome, a
//     constante, e os casts que a ABI crua da syscall (argumentos todos `long`) exige. `fd` e
//     `off` são por convenção `-1`/`0` para o mapeamento anônimo que o E1 usa (MAP_ANONYMOUS
//     os ignora), mas este wrapper é genérico sobre os dois -- não fixa essa suposição.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_mmap.h"
#include "syscall.h"
#include "syscall_nums.h"

// EN: TST-STATIC (TODO.md, W11): cppcheck flags `addr` as "can be declared as pointer to
//     const" (constParameterPointer) because this function body never writes through it. That
//     is technically true of the C statements below, but misses the actual contract: `addr` is
//     the POSIX mmap() hint address, part of the public signature declared in
//     include/sys_mmap.h (kept intentionally non-const to mirror mmap(2)/POSIX) -- narrowing it
//     to `const void*` here would desync this definition from that header's `void*` declaration
//     (a compile error) for zero real safety gain. Suppressed cirurgically, not the whole check.
// PT: TST-STATIC (TODO.md, W11): o cppcheck aponta `addr` como "pode ser declarado como
//     ponteiro pra const" (constParameterPointer) porque o corpo desta função nunca escreve
//     através dele. Isso é tecnicamente verdade das instruções C abaixo, mas ignora o contrato
//     real: `addr` é o endereço-sugestão do mmap() POSIX, parte da assinatura pública declarada
//     em include/sys_mmap.h (mantida deliberadamente não-const, espelhando mmap(2)/POSIX) --
//     estreitar pra `const void*` aqui dessincronizaria esta definição daquela declaração
//     `void*` do header (erro de compilação) sem ganho real de segurança. Suprimido
//     cirurgicamente, não a checagem inteira.
// cppcheck-suppress constParameterPointer
void* sys_mmap(void* addr, size_t len, int prot, int flags, int fd, long off) {
    return (void*)syscall6(SYS_mmap, (long)addr, (long)len, (long)prot,
                            (long)flags, (long)fd, off);
}
