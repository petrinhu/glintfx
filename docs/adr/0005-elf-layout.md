# ADR-0005 — ELF layout & link model / Layout ELF e modelo de link

- **Status:** Accepted (2026-06-21)
- **Deciders:** petrus (líder), software-architect (recomendação sem divergência)
- **Tags:** foundation, one-way-door

## Context / Contexto

**EN:** Without libc there is no `crt0`, no dynamic loader cooperation, no default entry. We must define how the final binary is laid out and linked.

**PT:** Sem libc não há `crt0`, nem cooperação com loader dinâmico, nem entry padrão. Precisamos definir como o binário final é organizado e linkado.

## Decision / Decisão

**EN:** Produce a **static, non-PIE ELF64** executable:
- Link: `ld -nostdlib -static -no-pie -e _start`.
- Compile: `clang -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-builtin`.
- Entry point: our own `_start` (see [[0003-internal-abi]]); no C runtime startup.
- Sections: standard `.text` / `.rodata` / `.data` / `.bss`; no dynamic relocations, no interpreter.
- Fixed load address (no-PIE) to keep startup trivial; no dynamic loader.

**PT:** Produzir um executável **ELF64 estático e não-PIE**:
- Link: `ld -nostdlib -static -no-pie -e _start`.
- Compilação: `clang -ffreestanding -nostdlib -fno-pic -fno-stack-protector -fno-builtin`.
- Entry point: nosso `_start` (ver [[0003-internal-abi]]); sem startup de runtime C.
- Seções: `.text` / `.rodata` / `.data` / `.bss` padrão; sem relocations dinâmicas, sem interpreter.
- Endereço de carga fixo (não-PIE) para manter o início trivial; sem loader dinâmico.

## Consequences / Consequências

**EN:** Simplest possible startup and debugging; the binary is fully self-contained. No ASLR for the main image (acceptable for a study runtime; revisit if a security surface appears). Larger on-disk size than dynamic linking — irrelevant here.

**PT:** O início e a depuração mais simples possíveis; o binário é totalmente autocontido. Sem ASLR para a imagem principal (aceitável num runtime de estudo; revisitar se surgir superfície de segurança). Tamanho em disco maior que com link dinâmico — irrelevante aqui.

## Options considered / Opções consideradas

- **Static non-PIE (chosen / escolhida)** — startup trivial, autocontido.
- PIE + dynamic / PIE + dinâmico — ASLR, mas exige loader e relocations. Rejected / Rejeitada (contraria "zero libs").
