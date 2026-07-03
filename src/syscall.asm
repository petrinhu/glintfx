; SPDX-License-Identifier: MPL-2.0
; EN: Implements syscall0..6 (ADR-0001). Each function receives its args per the System V
;     AMD64 C-ABI (1st..6th integer/pointer arg in rdi,rsi,rdx,rcx,r8,r9; a 7th spills to the
;     stack at [rsp+8]) and reshuffles them into the raw `syscall` instruction's own
;     convention (rax=number, args in rdi,rsi,rdx,r10,r8,r9 -- note r10 replaces rcx, which
;     the `syscall` instruction itself clobbers together with r11). Pure register glue, no
;     branches, no stack frame -- these are leaf functions.
; PT: Implementa syscall0..6 (ADR-0001). Cada funcao recebe seus args pela C-ABI System V
;     AMD64 (1o..6o arg inteiro/ponteiro em rdi,rsi,rdx,rcx,r8,r9; um 7o transborda pra pilha
;     em [rsp+8]) e os reorganiza pra convencao propria da instrucao `syscall` (rax=numero,
;     args em rdi,rsi,rdx,r10,r8,r9 -- nota: r10 substitui rcx, que a propria instrucao
;     `syscall` destroi junto com r11). Puro glue de registrador, sem desvio, sem frame de
;     pilha -- sao funcoes leaf.
; Copyright (c) 2026 Petrus Silva Costa

global syscall0
global syscall1
global syscall2
global syscall3
global syscall4
global syscall5
global syscall6

section .text
bits 64

syscall0:                  ; nr=rdi
    mov rax, rdi
    syscall
    ret

syscall1:                  ; nr=rdi a1=rsi
    mov rax, rdi
    mov rdi, rsi
    syscall
    ret

syscall2:                  ; nr=rdi a1=rsi a2=rdx
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    syscall
    ret

syscall3:                  ; nr=rdi a1=rsi a2=rdx a3=rcx
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    syscall
    ret

syscall4:                  ; nr=rdi a1=rsi a2=rdx a3=rcx a4=r8
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8             ; syscall wants a4 in r10, not r8
    syscall
    ret

syscall5:                  ; nr=rdi a1=rsi a2=rdx a3=rcx a4=r8 a5=r9
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8              ; read old r8 (a4) BEFORE r8 is overwritten below
    mov r8,  r9
    syscall
    ret

syscall6:                  ; nr=rdi a1=rsi a2=rdx a3=rcx a4=r8 a5=r9 a6=[rsp+8]
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8
    mov r8,  r9              ; read old r9 (a5) BEFORE r9 is overwritten below
    mov r9,  [rsp+8]          ; 7th C arg (a6), spilled to the stack by the caller
    syscall
    ret

section .note.GNU-stack noalloc noexec nowrite
