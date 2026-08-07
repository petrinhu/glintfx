# Contexto do projeto para a doc canônica de GitHub Actions (`DOC-GHA-CANONICO`)

**Leia este arquivo inteiro antes de escrever uma linha da sua seção.**

Projeto: **glintfx**, em `/home/petrus/IDrive/Documentos/projetos_claudebrain/Projects/loucura_c_asm`.

## 🔴 O que NÃO produzir: mais um tutorial genérico

A internet está cheia de "introdução ao GitHub Actions". Um doc que só reconta a documentação
oficial **não vale o espaço que ocupa** e envelhece pior que o original.

**O valor está em amarrar o que a documentação diz ao que ESTE projeto MEDIU.** Cada tópico que
você trouxer deve, sempre que houver ligação, apontar para um dos incidentes abaixo — todos reais,
todos de 2026-08-06, todos rastreáveis no `TODO.md` pelo ID citado.

## Os nossos 6 fluxos

`ci.yml` (gate de release) · `core-ci.yml` (Camada 0, C+ASM) · `heavy.yml` (ASan/UBSan e motor de
fonte, **runner self-hosted**) · `distro-matrix.yml` (Fedora 44 + Arch + Ubuntu em container) ·
`nightly.yml` (rede de segurança) · `windows-atoms.yml` (MSVC dedicado).

## As 7 lições medidas (amarre a sua seção a estas)

1. **`workflow_dispatch` faltando nos três fluxos mais importantes** (`CI-DISPATCH-MANUAL`).
   Numa pane de ~4h do GitHub Actions, evento de `push` **parou de criar execução**; o disparo
   manual continuou funcionando. Mas `ci.yml`, `core-ci.yml` e `windows-atoms.yml` não tinham o
   gatilho e devolviam `HTTP 422`. **Resiliência de CI só se mede quando o caminho normal cai** —
   um fluxo sem via manual é indistinguível de um com, enquanto tudo funciona.
2. **Ferramenta invocada em lote ESCONDE cobertura perdida** (`CI-TIDY-CRASH`). O `clang-tidy`
   recebia 51 arquivos de uma vez; um SIGSEGV (exit **139**) matava o processo e ninguém sabia
   quantos ficaram sem análise. Trocado por laço arquivo-a-arquivo: apareceram **5 arquivos que já
   reprovavam**. **Crash não reprova, esconde.** O resumo tem de imprimir
   `encontrados`/`analisados`/`falharam` **mesmo quando zero**, e **distinguir exit 139 (ferramenta
   morreu) de exit 1 (violação real)** — exigem ações opostas.
3. **Opção de compilador sem guarda de plataforma** (`CI-VERMELHO-2X`).
   `-Wno-missing-field-initializers` sem condição quebrou as duas pernas MSVC
   (`cl : command line error D8021`). **Quando o arquivo de build é compartilhado, o conjunto de
   plataformas a verificar é ENUMERADO, não lembrado** — o briefing dizia "as três pernas" e o
   Windows ficou de fora.
4. **Fixar ação por SHA é a escolha CERTA de segurança E cria fragilidade**
   (`CI-HEAVY-SEM-CACHE-DE-ACAO`). Runner self-hosted em repo público exige pin por SHA
   (supply-chain), mas obriga download novo a cada execução; três etapas falharam com timeout de
   100s no `codeload.github.com`. **Registrar trocas conscientes como trocas, não como defeito.**
5. **Runner self-hosted em repo público não pode ter gatilho `pull_request`** — já é prática nossa.
   A documentação explica por quê; amarre.
6. **Higiene de versão de ação** (`CI-ACTIONS-NODE20`): **16 pontos** usando ações que miram
   Node.js 20, hoje **forçadas** a Node 24. Funciona; quebra quando a ponte cair. Versões atuais
   medidas: `actions/checkout` **v7.0.1**, `actions/cache` **v6.1.0**.
7. **Diagnóstico: `Set up job` versus passo NOSSO.** Falha em `Set up job` é infraestrutura do
   GitHub, não código nosso. Distinguir isso salvou horas; confundir levaria a "consertar" o que
   não está quebrado. Ensine **como olhar** (`gh run view <id> --json jobs`, qual passo falhou)
   antes de concluir.

## Regras de método, todas obrigatórias

- **Não invente comportamento do GitHub.** Não achou na documentação? Escreva **"não confirmado na
  documentação"**. Resultado negativo declarado vale mais que afirmação plausível — nesta sessão o
  orquestrador deu duas explicações inventadas que a medição desmentiu.
- ⚠️ **Contradição entre a documentação e a nossa prática é o achado MAIS VALIOSO que você pode
  trazer.** Não silencie para o doc ficar coerente.
- **Denominador declarado:** diga quantas páginas você leu **de fato**, e quais deixou de fora com
  o motivo. "Li a seção" sem número não é medição.
- **Citação por número de linha apodrece** (o gate deste repo já acusa 67 citações deslocadas).
  Ao citar nossos fluxos, prefira **nome do job + arquivo**; linha só como auxiliar.
- **Bilíngue no MESMO arquivo, en-intl primeiro, depois pt-br** — convenção do projeto para tudo
  em `docs/`.
- **Grave incrementalmente.** Página lida é página registrada; não acumule para escrever no fim.
  Dois agentes desta sessão perderam trabalho por deixá-lo só no contexto.

## Onde escrever

**Cada agente escreve SÓ o seu arquivo** em `/var/tmp/gha-docs/`. Não toque no arquivo de outro.
A consolidação em `docs/github-actions.md` é de um agente designado, **depois** que as quatro
seções fecharem.

| seção | arquivo |
|---|---|
| Referência (sintaxe, contextos, expressões) | `secao-referencia.md` |
| Escrever fluxos (gatilhos, jobs, matriz, cache, artefatos, reuso, ações próprias) | `secao-escrever.md` |
| Segurança e executores auto-hospedados | `secao-seguranca.md` |
| Gerenciar execuções, monitorar e diagnosticar | `secao-operacao.md` |

## Fora de escopo, declarado

"Migrar para o GitHub Actions" (vindo de Jenkins/Travis/CircleCI) — irrelevante para este projeto.
**Não leia**, e registre a exclusão no seu relatório para o denominador ficar honesto.

## Restrições duras

- **Não altere nenhum fluxo.** Isto é documentação. Achou defeito? **Reporte.**
- ⚠️ Outros agentes estão ativos em `glintfx/src/rml/`, `glintfx/tests/uix_style/`, `.clang-tidy` e
  `glintfx/src/rml/rcss_dump_corpus_sanity.cpp`. **Não toque em nada disso.**
- **NÃO faça commit, NÃO faça push, NÃO taggeie.** Só escreva o seu arquivo em `/var/tmp/gha-docs/`.
