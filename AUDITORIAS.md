# AUDITORIAS — loucura_c_asm

Manual de auditorias do projeto. Auditoria é **downstream** de código + teste: só audita o que já foi implementado e testado. Stack: C + Assembly puros, sem libc, Linux x86-64. Itens `AUD-*` na tabela.

## Catálogo aplicável

| ID | Auditoria | O que verifica | Pré-requisito |
| :--- | :--- | :--- | :--- |
| AUD-ABI | Conformidade ABI / ASM | Convenção de chamada System V AMD64 ([ADR-0003](docs/adr/0003-internal-abi.md)): preservação de registradores callee-saved (`rbx,rbp,r12-r15`), alinhamento de stack 16B em todo `call`, uso correto de `r10` no `syscall`, ausência de clobber indevido | Código de bootstrap + libc-núcleo testados |
| AUD-SEC | Segurança | Validação de argumentos de syscall, checagem de bounds em mem/string, overflow inteiro em conversões e no alocador, ausência de UB explorável; superfície de ataque (entrada não-confiável) | Núcleo testado |

## Severidade dos achados

- **CRÍTICO** — corrompe memória/estado, viola ABI silenciosamente, ou é explorável. Bloqueia release.
- **IMPORTANTE** — bug latente, invariante frágil, falta de validação. Corrigir antes da tag.
- **COSMÉTICO** — estilo, clareza, doc. Não bloqueia.

## Regra

Nenhum item vai para `REL-TAG` (tag de versão) com achado **CRÍTICO** aberto. Estado Auditado na tabela: `✓` aprovado · `⚠` com ressalvas · `—` não auditado. Auditoria sempre tem como pré-requisito os itens de código + teste que ela cobre (nunca auditar antes de testar).
