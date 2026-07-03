; SPDX-License-Identifier: MPL-2.0
; EN: Our own process entry point (ADR-0003, ADR-0005) -- there is no crt0/crt1 from any
;     libc. The kernel jumps here directly after execve(); NOTHING has run before this. Per
;     the System V AMD64 ABI (Linux specifics), at process entry:
;       [rsp]                = argc
;       [rsp+8]               = argv[0]  (an array of argc pointers, NULL-terminated)
;       [rsp+8+8*(argc+1)]    = envp[0]  (an array of pointers, NULL-terminated)
;     _start reads all three, aligns the stack to 16 bytes (ADR-0003 -- defensive: the ABI
;     already guarantees this at process entry, but we do not trust the loader
;     unconditionally), and calls `main(argc, argv, envp)`. If main ever returns, its return
;     value (in eax) is forwarded to the SYS_exit syscall via syscall1 (B3) -- this is the
;     ONLY termination path for a program that does not call sys_exit() itself; _start must
;     NEVER fall through past this point (`ret` here would jump into whatever garbage follows
;     in memory -- there is no caller to return to).
; PT: Nosso proprio ponto de entrada de processo (ADR-0003, ADR-0005) -- nao ha crt0/crt1 de
;     libc nenhuma. O kernel salta pra ca diretamente apos o execve(); NADA rodou antes disso.
;     Pela ABI System V AMD64 (especificidades Linux), na entrada do processo:
;       [rsp]                = argc
;       [rsp+8]               = argv[0]  (array de argc ponteiros, terminado em NULL)
;       [rsp+8+8*(argc+1)]    = envp[0]  (array de ponteiros, terminado em NULL)
;     O _start le os tres, alinha a pilha em 16 bytes (ADR-0003 -- defensivo: a ABI ja
;     garante isso na entrada do processo, mas nao confiamos incondicionalmente no loader), e
;     chama `main(argc, argv, envp)`. Se main algum dia retornar, seu valor de retorno (em
;     eax) e repassado a syscall SYS_exit via syscall1 (B3) -- esse e o UNICO caminho de
;     terminacao pra um programa que nao chama sys_exit() por conta propria; o _start NUNCA
;     pode cair alem deste ponto (`ret` aqui saltaria pro lixo que estiver depois na memoria
;     -- nao ha chamador pra quem retornar).
; Copyright (c) 2026 Petrus Silva Costa

%include "syscall_nums.inc"

extern main
extern syscall1
global _start

section .text
bits 64

_start:
    xor  rbp, rbp             ; EN: zero frame pointer -- conventional "end of stack" marker
                               ;     for debuggers/unwinders (glibc's crt1 does the same).
                               ; PT: zera o frame pointer -- marcador convencional de "fim da
                               ;     pilha" pra debuggers/unwinders (o crt1 da glibc faz igual).

    mov  rdi, [rsp]            ; argc
    lea  rsi, [rsp+8]          ; argv = &argv[0]
    lea  rdx, [rsi+rdi*8+8]    ; envp = argv + (argc+1)*8  (skip argv's own NULL terminator)

    and  rsp, -16              ; EN: force 16-byte alignment before `call main` (ADR-0003).
                                ; PT: forca alinhamento de 16 bytes antes de `call main`
                                ;     (ADR-0003).

    call main                  ; main(argc, argv, envp) -- rdi/rsi/rdx already set above

    ; EN: Fallback termination path -- reached only if main() actually returns (a program
    ;     that explicitly calls sys_exit()/_exit(), like the B5 exit42 gate program, never
    ;     gets here).
    ; PT: Caminho de terminacao de reserva -- alcancado so se main() de fato retornar (um
    ;     programa que chama sys_exit()/_exit() explicitamente, como o programa-gate exit42
    ;     da B5, nunca chega aqui).
    movsxd rsi, eax             ; a1 = (long)main's return value
    mov    rdi, SYS_exit         ; nr = 60
    call   syscall1

    ud2                          ; EN: unreachable -- SYS_exit never returns; trap loudly if
                                  ;     it somehow did.
                                  ; PT: inalcancavel -- SYS_exit nunca retorna; trapeia alto se,
                                  ;     por algum motivo, retornasse.

section .note.GNU-stack noalloc noexec nowrite
