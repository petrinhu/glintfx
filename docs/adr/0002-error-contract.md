# ADR-0002 — Error contract / Contrato de erro

- **Status:** Accepted (2026-06-21)
- **Deciders:** petrus (líder), software-architect
- **Tags:** foundation, one-way-door

## Context / Contexto

**EN:** Linux syscalls report failure by returning a small negative value in `rax` (e.g. `-2` = `ENOENT`); userland libc normally translates that into `-1` + a global `errno`. Without libc we choose our own convention.

**PT:** Syscalls Linux reportam falha retornando um valor negativo pequeno em `rax` (ex.: `-2` = `ENOENT`); a libc normalmente traduz isso em `-1` + um `errno` global. Sem libc, escolhemos nossa própria convenção.

## Decision / Decisão

**EN:** Propagate the **raw `-errno` return**. Functions return the kernel value directly; callers test `if (ret < 0)` and read `-ret` as the error code. No global `errno`, no hidden state.

**PT:** Propagar o **retorno `-errno` cru**. As funções retornam o valor do kernel diretamente; quem chama testa `if (ret < 0)` e lê `-ret` como o código de erro. Sem `errno` global, sem estado oculto.

## Consequences / Consequências

**EN:** Naturally thread-safe and reentrant (no shared mutable state). Slightly less familiar than classic `errno`; we'll provide small helpers/macros (e.g. `IS_ERR(x)`, `ERR_CODE(x)`) for clarity. Error codes live in our own header (no `<errno.h>`).

**PT:** Naturalmente thread-safe e reentrante (sem estado mutável compartilhado). Um pouco menos familiar que o `errno` clássico; forneceremos pequenos helpers/macros (ex.: `IS_ERR(x)`, `ERR_CODE(x)`) para clareza. Os códigos de erro vivem em header próprio (sem `<errno.h>`).

## Options considered / Opções consideradas

- **Raw `-errno` return (chosen / escolhida)** — sem estado global, thread-safe.
- Global `errno` + `-1` / `errno` global + `-1` — familiar, mas estado global (atrito futuro com threads). Rejected / Rejeitada.

Cross-ref: [[0001-syscall-layer-style]].
