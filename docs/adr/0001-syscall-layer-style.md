# ADR-0001 — Syscall layer style / Estilo da camada de syscall

- **Status:** Accepted (2026-06-21)
- **Deciders:** petrus (líder), software-architect
- **Tags:** foundation, one-way-door

## Context / Contexto

**EN:** With no libc, every kernel interaction goes through the raw `syscall` instruction (System V AMD64: number in `rax`, args in `rdi, rsi, rdx, r10, r8, r9`, return in `rax`). We must decide how the codebase exposes this primitive.

**PT:** Sem libc, toda interação com o kernel passa pela instrução `syscall` crua (System V AMD64: número em `rax`, argumentos em `rdi, rsi, rdx, r10, r8, r9`, retorno em `rax`). Precisamos decidir como a base de código expõe essa primitiva.

## Decision / Decisão

**EN:** Use **generic arity-based wrappers** `syscall0`..`syscall6` written once in NASM. Named, typed C helpers (`sys_write`, `sys_read`, `sys_mmap`, ...) are thin functions layered on top of these. The assembly surface stays minimal and DRY; readability lives in the C layer.

**PT:** Usar **wrappers genéricos por aridade** `syscall0`..`syscall6`, escritos uma única vez em NASM. Helpers C nomeados e tipados (`sys_write`, `sys_read`, `sys_mmap`, ...) são funções finas por cima desses. A superfície em Assembly fica mínima e DRY; a legibilidade mora na camada C.

## Consequences / Consequências

**EN:** Adding a new syscall = one small C wrapper, no new assembly. The six wrappers must move `rcx`→`r10` (kernel clobbers `rcx`/`r11`) and preserve the ABI. Slightly less self-documenting at the asm level — mitigated by the C helpers.

**PT:** Adicionar uma nova syscall = um pequeno wrapper C, sem novo assembly. Os seis wrappers devem mover `rcx`→`r10` (o kernel destrói `rcx`/`r11`) e preservar a ABI. Um pouco menos autoexplicativo no nível asm — mitigado pelos helpers C.

## Options considered / Opções consideradas

- **Generic `syscall0..6` (chosen / escolhida)** — DRY, flexível.
- One dedicated asm wrapper per syscall / Um wrapper asm dedicado por syscall — mais legível, muito mais código repetido. Rejected / Rejeitada.

Cross-ref: [[0003-internal-abi]].
