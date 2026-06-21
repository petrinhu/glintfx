# TESTES — loucura_c_asm

Manual de testes do projeto. Stack: C + Assembly puros, freestanding, sem libc, Linux x86-64. Como não há framework externo, **o harness de teste é nosso** (item `C1`). Testes unitários seguem TDD e ridem junto da implementação (não são itens da tabela); este manual cobre os testes **não-unitários** (itens `TST-*`).

## Princípios

- Sem libc → sem framework de teste pronto. O runner (`C1`) reporta via `sys_write` e sinaliza resultado pelo **exit code** (0 = passou, ≠0 = falhou).
- Todo binário/demo deve ser determinístico e validável por exit code e/ou stdout.
- Testes rodam no CI freestanding (`F1`) e localmente via alvo `make test`.

## Catálogo aplicável

| ID | Tipo | O que cobre | Como |
| :--- | :--- | :--- | :--- |
| TST-INT | Integração / E2E | Cada binário/demo builda, roda e produz exit code + stdout esperados | Script que compila, executa e compara saída/`$?` (golden files) |
| TST-STATIC | Análise estática | Bugs sem executar: UB, uso indevido, dead code | `clang --analyze` / `scan-build`; cppcheck em modo freestanding |
| TST-MEM | Memória (alocador) | Leak, double-free, alinhamento, escrita fora de bloco no nosso `malloc` | Testes dedicados do alocador + `valgrind` no binário estático quando viável; checagem de invariantes do bump/free-list |

## Fora de escopo (por enquanto)

- Sanitizers (ASan/UBSan) exigem runtime próprio em freestanding — reavaliar se virar necessário.
- Fuzzing — considerar quando houver parser de input não-confiável (gatilho de re-roteamento do porte).

## Definition of Done de teste

Um item de implementação só vira `✅ Concluído` depois que os `TST-*` da sua onda passam. Implementação entregue mas sem teste verde fica `🔍 Pendente verificação`.
