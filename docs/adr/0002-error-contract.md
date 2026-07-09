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

**EN:** Naturally thread-safe and reentrant (no shared mutable state). Slightly less familiar than classic `errno`; we'll provide small helpers/macros (e.g. `IS_ERR(x)`, `ERR_CODE(x)`) for clarity. Error codes live in our own header (no `<errno.h>`). (see Addendum 2026-07-09)

**PT:** Naturalmente thread-safe e reentrante (sem estado mutável compartilhado). Um pouco menos familiar que o `errno` clássico; forneceremos pequenos helpers/macros (ex.: `IS_ERR(x)`, `ERR_CODE(x)`) para clareza. Os códigos de erro vivem em header próprio (sem `<errno.h>`). (ver Adendo 2026-07-09)

## Options considered / Opções consideradas

- **Raw `-errno` return (chosen / escolhida)** — sem estado global, thread-safe.
- Global `errno` + `-1` / `errno` global + `-1` — familiar, mas estado global (atrito futuro com threads). Rejected / Rejeitada.

Cross-ref: [[0001-syscall-layer-style]].

## Addendum (2026-07-09)

**EN:** Audit finding `AUD-C0-4` confirmed by grep that the `IS_ERR(x)`/`ERR_CODE(x)` helpers and the "own header" for error codes promised above were never implemented anywhere in the repo. This is not a functional bug — the underlying Decision (raw `-errno` propagation) is followed correctly at every call site — but the Consequences section had drifted from reality.

Current practice: each call site applies the `-errno` contract inline, ad hoc (e.g. `src/alloc.c`'s `mmap_failed` checks the returned pointer range; `tests/echo_stdin.c` checks `n < 0`). We are **deliberately deferring** the helpers/macros and the dedicated error-code header — YAGNI. With only two consumers of the pattern today, extracting an abstraction now would be premature; it adds a header and an indirection layer to maintain without a third use case to validate the shape of the API.

**Extraction trigger:** revisit this once there are **3 or more** call sites that would benefit from a shared `IS_ERR`/`ERR_CODE` helper and/or a common error-code header. This ADR's Status and Decision remain unchanged (raw `-errno` propagation is still ratified and in force); only the Consequences' promise of helpers/header is corrected here.

**PT:** O achado de auditoria `AUD-C0-4` confirmou via grep que os helpers `IS_ERR(x)`/`ERR_CODE(x)` e o "header próprio" de códigos de erro prometidos acima nunca foram implementados em lugar nenhum do repo. Não é um bug funcional — a Decisão de fundo (propagação de `-errno` cru) é seguida corretamente em todos os call sites —, mas a seção Consequências havia divergido da realidade.

Prática atual: cada call site aplica o contrato `-errno` inline, ad hoc (ex.: `mmap_failed` em `src/alloc.c` checa a faixa do ponteiro retornado; `tests/echo_stdin.c` checa `n < 0`). Estamos **adiando deliberadamente** os helpers/macros e o header dedicado de códigos de erro — YAGNI. Com apenas dois consumidores do padrão hoje, extrair uma abstração agora seria prematuro; adiciona um header e uma camada de indireção para manter sem um terceiro caso de uso que valide o formato da API.

**Gatilho de extração:** revisitar isto quando houver **3 ou mais** call sites que se beneficiariam de um helper `IS_ERR`/`ERR_CODE` compartilhado e/ou um header comum de códigos de erro. O Status e a Decisão deste ADR permanecem inalterados (a propagação de `-errno` cru continua ratificada e em vigor); só a promessa de helpers/header nas Consequências é corrigida aqui.
