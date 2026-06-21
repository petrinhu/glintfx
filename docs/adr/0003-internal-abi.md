# ADR-0003 — Internal ABI / ABI interna

- **Status:** Accepted (2026-06-21)
- **Deciders:** petrus (líder), software-architect
- **Tags:** foundation, one-way-door

## Context / Contexto

**EN:** C and Assembly must call each other. We need one calling convention so clang-generated code and hand-written NASM interoperate without surprises.

**PT:** C e Assembly precisam se chamar mutuamente. Precisamos de uma convenção de chamada única para que o código gerado pelo clang e o NASM escrito à mão interoperem sem surpresas.

## Decision / Decisão

**EN:** Adopt the **strict System V AMD64 ABI** (what clang already assumes):
- Integer/pointer args: `rdi, rsi, rdx, rcx, r8, r9`; return in `rax` (`rdx:rax` for 128-bit).
- Callee-saved: `rbx, rbp, rsp, r12, r13, r14, r15`. Caller-saved: the rest.
- Stack 16-byte aligned at every `call` boundary; `_start` fixes alignment before calling `main`.
- Note: the `syscall` instruction uses `r10` (not `rcx`) for the 4th arg and clobbers `rcx`/`r11` — handled inside the wrappers (see [[0001-syscall-layer-style]]).

**PT:** Adotar a **ABI System V AMD64 estrita** (o que o clang já assume):
- Args inteiros/ponteiros: `rdi, rsi, rdx, rcx, r8, r9`; retorno em `rax` (`rdx:rax` para 128 bits).
- Preservados pelo chamado: `rbx, rbp, rsp, r12, r13, r14, r15`. Os demais são do chamador.
- Stack alinhada em 16 bytes em toda fronteira de `call`; o `_start` corrige o alinhamento antes de chamar `main`.
- Nota: a instrução `syscall` usa `r10` (não `rcx`) para o 4º arg e destrói `rcx`/`r11` — tratado dentro dos wrappers (ver [[0001-syscall-layer-style]]).

## Consequences / Consequências

**EN:** Zero-friction C↔ASM interop; can mix freely. We must respect callee-saved registers in hand-written asm or face subtle corruption. Locks us to x86-64 SysV (acceptable: single-arch target).

**PT:** Interop C↔ASM sem atrito; podemos misturar livremente. Devemos respeitar os registradores preservados no asm à mão, sob pena de corrupção sutil. Nos prende ao SysV x86-64 (aceitável: alvo de arquitetura única).

## Options considered / Opções consideradas

- **Strict SysV AMD64 (chosen / escolhida)** — interop nativo com clang.
- Custom ABI / ABI própria — mais controle em hot paths, perde interop natural. Rejected / Rejeitada.
