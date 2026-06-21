# ADR-0004 — Allocator strategy / Estratégia do alocador

- **Status:** Accepted (2026-06-21)
- **Deciders:** petrus (líder), software-architect
- **Tags:** foundation, one-way-door

## Context / Contexto

**EN:** No libc means no `malloc`. We need dynamic memory from the kernel (`mmap`/`brk`) and a public allocation API. The internal algorithm can evolve; the **API signature cannot** without breaking every caller.

**PT:** Sem libc não há `malloc`. Precisamos de memória dinâmica do kernel (`mmap`/`brk`) e de uma API pública de alocação. O algoritmo interno pode evoluir; a **assinatura da API não pode**, sob pena de quebrar todos os chamadores.

## Decision / Decisão

**EN:** Start with a **bump allocator over `mmap`**, but commit to the **final signature now**: `malloc/free/realloc` (POSIX-shaped). Initially `free` may be a no-op and `realloc` may copy; the internal strategy upgrades to a free-list later **without changing the API**.

**PT:** Começar com um **bump allocator sobre `mmap`**, mas fixar a **assinatura final desde já**: `malloc/free/realloc` (no formato POSIX). Inicialmente `free` pode ser no-op e `realloc` pode copiar; a estratégia interna evolui para free-list depois **sem mudar a API**.

## Consequences / Consequências

**EN:** Unblocks higher layers immediately with minimal code; early memory is effectively leak-by-design until the free-list lands (acceptable for bootstrap, flagged in `TST-MEM`). The final signature being fixed avoids a costly downstream refactor (this is why it's one-way-door).

**PT:** Desbloqueia as camadas superiores de imediato com código mínimo; a memória inicial é efetivamente leak-by-design até a free-list chegar (aceitável no bootstrap, sinalizado em `TST-MEM`). A assinatura final fixada evita um refactor caro a jusante (por isso é one-way-door).

## Options considered / Opções consideradas

- **Bump over `mmap`, final signature (chosen / escolhida)** — simples agora, API estável.
- Full free-list from day one / Free-list completa desde o dia 1 — completo, mas complexo e arriscado antes de testes maduros. Rejected / Rejeitada.

Cross-ref: [[0002-error-contract]] (mmap failure → `-errno`).
