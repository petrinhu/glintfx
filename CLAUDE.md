# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> Estado em 2026-06-21: projeto recém-criado. Só existe o scaffold (pastas + arquivos git). Ainda **não há código-fonte**. As seções de build e arquitetura abaixo descrevem as **decisões já tomadas** e como o projeto DEVE ser construído — não código existente. Atualize conforme o código nascer.

## O que é este projeto

`loucura_c_asm` é um projeto **sério em C + Assembly puros**, com uma restrição central e inegociável:

- **ZERO bibliotecas. Sem libc, sem stdio, sem nada.** Nem `printf`, nem `malloc`, nem string ops prontas.
- O que faltar, **construímos do zero** (esta é a "loucura" do nome). A nossa própria substituta da libc vive no repo.
- Quando precisarmos de algo conhecido, **pesquisar na web sob demanda** libs públicas e papers → baixar → estudar → fazer RE (reverse engineering) → **reimplementar a nossa própria versão**. Não linkar a externa.

Consequência prática: a única ponte com o mundo é a **interface de syscalls do kernel Linux**. Toda I/O, alocação de memória e tempo passam por `syscall` direto.

## Arquitetura em camadas (escopo expandido 2026-06-28 — ver ADR-0006)

O projeto agora tem **duas camadas**. A filosofia "zero libs / sem libc" abaixo descreve a **Camada 0**, não o repo inteiro.

- **Camada 0 — núcleo soberano (C + ASM, intacto):** o runtime freestanding zero-libc descrito neste documento. ADR 0001–0005 valem aqui. É o **alvo de internalização** de longo prazo.
- **Camada 1 — biblioteca C++23 (a entrega):** lib compatível **C++17→C++23** que **une RmlUi** (UI HTML/CSS + layout) ao **renderer GL3** com todos os efeitos (glow, degradê, blur, drop-shadow, mask, backdrop-filter). Começa **linkando** RmlUi + gl3w + libGL + janela (GLFW primário; depois SDL, X11). Build via **CMake**. ASM só em hot paths.
- **Trajetória ("loucura"):** internalizar peças da Camada 1 com reimplementações **clean-room** sobre a Camada 0, com o tempo.
- **Estudo/RE:** upstreams clonados em `examples/` (gitignored): `examples/RmlUi` (MIT), `examples/gl3w` (domínio público). Não copiar código — reimplementar clean-room (base é MPL-2.0).

## Stack e alvo (decidido, não negociável sem o líder)

> Esta tabela descreve a **Camada 0**. A Camada 1 (C++23) usa CMake + RmlUi/gl3w/libGL + GLFW/SDL/X11 — ver ADR-0006.

| Item | Escolha |
|---|---|
| Plataforma | Linux **x86-64**, ELF nativo. Roda sobre o kernel (não é bare-metal). |
| Entry point | `_start` próprio. **Não** há `crt0`/`main` da libc. `exit` é via syscall. |
| I/O / serviços | `syscall` direta (`write`=1, `read`=0, `exit`=60, `mmap`=9, ...). ABI System V AMD64. |
| Assembler | **NASM**, sintaxe **Intel**, arquivos `.asm` (`-f elf64`). |
| Compilador C | **clang**, `-std=c23 -ffreestanding -nostdlib`. |
| Linker | **ld** direto (GNU ld), `-nostdlib -static -no-pie`. |
| Arquitetura | só **x86-64** por ora. |

## Como buildar (sem libc)

Ainda não há `Makefile` — quando o primeiro código surgir, consolidar estes comandos nele. Comandos canônicos:

```sh
# Compilar C freestanding (sem libc, sem PIC, sem stack protector)
clang -std=c23 -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
      -fno-builtin -fno-asynchronous-unwind-tables -m64 -Wall -Wextra \
      -Iinclude -c src/arquivo.c -o build/arquivo.o

# Montar Assembly (NASM, Intel, ELF64)
nasm -f elf64 src/arquivo.asm -o build/arquivo.o

# Linkar sem libc, entry _start, binario estatico nao-PIE
ld -nostdlib -static -no-pie -e _start -o build/programa build/*.o

# Rodar e ver o codigo de saida (vem da syscall exit)
./build/programa; echo "exit=$?"
```

Por que essas flags importam (sem libc):
- `-nostdlib` / `ld -nostdlib`: não linka libc nem `crt*.o` de startup → por isso precisamos do nosso `_start`.
- `-ffreestanding`: diz ao clang que não há ambiente hospedado; ele não pode assumir que funções da libc existem.
- `-fno-builtin`: impede o clang de trocar, p.ex., um laço por uma chamada a `memcpy`/`memset` da libc (que não temos). Se quisermos `memcpy`, é a NOSSA.
- `-no-pie` / `-fno-pic`: binário em endereço fixo simplifica o início; PIE exigiria um loader dinâmico.

## Inspeção / debug (read-only, sempre úteis)

```sh
objdump -d -M intel build/programa     # desmontar (Intel, combina com NASM)
readelf -a build/programa              # cabecalhos ELF, secoes, simbolos
gdb build/programa                     # 'layout asm', 'starti', 'info registers'
strace ./build/programa                # ver as syscalls de verdade (auditar a nossa "libc")
```

## Estrutura de pastas

```
src/      Codigo-fonte (.c e .asm). Inclui a nossa runtime/substituta-de-libc.
include/  Headers da API que criarmos (.h e .inc do NASM).
tests/    Testes (a definir o harness — provavelmente nosso proprio runner).
docs/     Notas de RE, papers baixados, decisoes (ADR), referencias de syscall/ABI.
tools/    Scripts auxiliares (build, dump, automacao de RE).
build/    Artefatos (.o, binarios). Ignorado pelo git.
```

## Convenções

Regras de idioma (decididas pelo líder):

- **Identificadores de código** (funções, variáveis, macros, labels NASM, structs): **apenas en-intl** (inglês internacional). Nada de pt-br em nome de símbolo.
- **Documentação** (arquivos em `docs/`, README, ADRs, e doc-comments / cabeçalhos de arquivo e de função): **bilíngue no MESMO arquivo — en-intl primeiro, depois pt-br.**
- **CLAUDE.md e memórias** (instrução operacional para o agente/líder) seguem em **pt-br** — não são "docs do produto".
- **Comunicação no chat** com o líder: **pt-br**.

Outras:

- Commits em **Conventional Commits**; mensagem em pt-br.
- Assembly sempre em sintaxe **Intel** (consistência com NASM e com `objdump -M intel`).

## Regras de trabalho (deste projeto)

- **Não introduzir nenhuma dependência externa nem chamada à libc.** Se a tentação aparecer, a resposta é implementar do zero. Em caso de dúvida real, perguntar ao líder.
- Decisões de arquitetura/stack são do líder (petrus): apresentar 2–3 opções com prós/contras antes de decidir (ver memória do projeto).
- Calibre de comunicação ao explicar: C/ASM com mais contexto; arquitetura, hardware e lógica direto ao ponto (ver memória `feedback-nivel-comunicacao`).

## Licença

**MPL-2.0** (Mozilla Public License 2.0). Texto em `LICENSE`; atribuições das deps linkadas em `NOTICE`. © 2026 Petrus Silva Costa. SPDX: `MPL-2.0`. Copyleft **fraco por-arquivo** (modificações nos NOSSOS arquivos voltam abertas; apps de terceiros — inclusive proprietários — podem linkar a lib livremente) + grant de patente. Aplicada ao projeto inteiro; **relicenciada de AGPL-3.0 em 2026-06-28** (cliente da Camada 1 é adoção externa — ver ADR-0007). Ao reimplementar/RE de libs externas, **não copiar código** — reimplementar clean-room (a partir do entendimento).

## Pendências

A tabela de pendências e planejamento está em `TODO.md` na raiz (ordenada por execução; coluna Onda marca passos paralelizáveis). Decisões de arquitetura em `docs/adr/`. Manuais de teste e auditoria em `TESTES.md` e `AUDITORIAS.md`. Governança/porte em `.bigtech-porte`; a definição completa da constelação (agents, RACI, pipelines) está no plugin `bigtech_plugin` ([Codeberg](https://codeberg.org/petrinhu/bigtech_plugin) · [GitHub](https://github.com/petrinhu/bigtech_plugin)).

## Ambiente (Fedora 44)

Toolchain instalado e verificado: **nasm 3.01**, **clang 22**, GNU **ld/objdump/readelf 2.46**, **gdb 17**, **make 4.4**.
`qemu` não está instalado e não é necessário (alvo é Linux nativo, não bare-metal).
