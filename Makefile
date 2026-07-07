# SPDX-License-Identifier: MPL-2.0
# EN: Freestanding build system for the sovereign C+ASM runtime (Layer 0, "loucura_c_asm").
#     Zero libc: every flag below explicitly drops the toolchain's own runtime assumptions.
#     See CLAUDE.md "Como buildar (sem libc)" for why each flag exists. Wildcard-based:
#     src/*.c + src/*.asm are the shared runtime, linked into EVERY tests/*.c program (each
#     one its own `main`). Adding a new runtime primitive (src/) or a new gate program
#     (tests/) needs NO edit here -- picked up automatically.
# PT: Sistema de build freestanding do runtime soberano C+ASM (Camada 0, "loucura_c_asm").
#     Zero libc: toda flag abaixo derruba explicitamente as suposicoes de runtime do
#     toolchain. Ver CLAUDE.md "Como buildar (sem libc)" pra por que cada flag existe.
#     Wildcard-based: src/*.c + src/*.asm sao a runtime compartilhada, linkada em TODO
#     programa tests/*.c (cada um com seu proprio `main`). Adicionar uma primitiva nova
#     (src/) ou um programa-gate novo (tests/) NAO exige editar este arquivo -- e' pego
#     automaticamente.
# Copyright (c) 2026 Petrus Silva Costa

CC := clang
AS := nasm
LD := ld

CFLAGS  := -std=c23 -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
           -fno-builtin -fno-asynchronous-unwind-tables -m64 -Wall -Wextra -Iinclude
ASFLAGS := -f elf64 -Iinclude/
LDFLAGS := -nostdlib -static -no-pie -e _start

BUILD := build
OBJ   := $(BUILD)/obj
BIN   := $(BUILD)/bin

# EN: Shared runtime objects -- linked into EVERY program below (the "libc" we are building).
# PT: Objetos de runtime compartilhados -- linkados em TODO programa abaixo (a "libc" que
#     estamos construindo).
RUNTIME_C_SRCS   := $(wildcard src/*.c)
RUNTIME_ASM_SRCS := $(wildcard src/*.asm)
RUNTIME_OBJS     := $(patsubst src/%.c,$(OBJ)/%.o,$(RUNTIME_C_SRCS)) \
                     $(patsubst src/%.asm,$(OBJ)/%.o,$(RUNTIME_ASM_SRCS))

# EN: Each tests/*.c file is its own tiny freestanding program (defines `main`), linked with
#     every runtime object above into build/bin/<name>. Known footgun (accepted, YAGNI):
#     a src/ file and a tests/ file sharing the same base name would collide under
#     build/obj/<name>.o -- do not duplicate base names across the two directories.
# PT: Cada tests/*.c e' um pequeno programa freestanding proprio (define `main`), linkado com
#     todo objeto de runtime acima em build/bin/<name>. Footgun conhecido (aceito, YAGNI):
#     um arquivo em src/ e outro em tests/ com o MESMO nome-base colidiriam em
#     build/obj/<name>.o -- nao duplicar nomes-base entre as duas pastas.
PROGRAM_SRCS := $(wildcard tests/*.c)
PROGRAMS     := $(patsubst tests/%.c,$(BIN)/%,$(PROGRAM_SRCS))

.PHONY: build test clean run

build: $(PROGRAMS)

$(OBJ)/%.o: src/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.asm | $(OBJ)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ)/%.o: tests/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/%: $(OBJ)/%.o $(RUNTIME_OBJS) | $(BIN)
	$(LD) $(LDFLAGS) -o $@ $^

$(OBJ) $(BIN):
	mkdir -p $@

# EN: Runs every program and checks its exit code against the manifest tests/expected_exit.txt
#     (lines "<name> <expected-code>"; missing entry defaults to 0). Temporary harness for this
#     increment -- superseded by the C1 test runner (TODO.md, W6) once it exists. Stdin is
#     explicitly redirected from /dev/null (B7): none of the programs in this manifest need
#     real input today, but leaving stdin attached to whatever invoked `make` (a human's
#     terminal, in particular) would make a future stdin-reading gate program (e.g. B7's
#     echo_stdin) block indefinitely waiting for input instead of seeing an immediate EOF.
#     Explicit > implicit: this makes "no input" a documented guarantee, not an accident of
#     how `make test` happened to be invoked.
# PT: Roda todo programa e checa o exit code contra o manifesto tests/expected_exit.txt
#     (linhas "<nome> <codigo-esperado>"; entrada ausente assume 0). Harness temporario deste
#     incremento -- substituido pelo runner de teste C1 (TODO.md, W6) quando existir. O stdin
#     e' explicitamente redirecionado de /dev/null (B7): nenhum programa deste manifesto
#     precisa de entrada real hoje, mas deixar o stdin herdado de quem chamou `make` (o
#     terminal de um humano, em particular) faria um futuro programa-gate que le stdin (ex.:
#     o echo_stdin da B7) travar indefinidamente esperando entrada em vez de ver um EOF
#     imediato. Explicito > implicito: isso torna "sem entrada" uma garantia documentada, nao
#     um acidente de como o `make test` foi chamado.
# EN: AUD-C0-3 (AUDIT_FIND.md) extension: some gate programs (today: printf_e2e, see its file
#     header) exercise the real stdout I/O path -- not just the exit code -- and need their
#     output verified byte-exact against a golden file, not just eyeballed with `od -c`. A second
#     manifest, tests/expected_stdout.txt ("<name> <golden-path>", same lookup shape as
#     expected_exit.txt), opts a program IN to this stricter check; a program absent from it
#     keeps the original exit-code-only contract unchanged. When a golden entry exists, stdout is
#     redirected to a temp file (never a shell command-substitution `$(...)`, which would eat any
#     trailing newline(s) and silently corrupt the byte-exact comparison) and diffed with `cmp
#     -s` -- POSIX `cmp` with no output, exit 0 iff every byte matches.
# PT: Extensao do AUD-C0-3 (AUDIT_FIND.md): alguns programas-gate (hoje: printf_e2e, ver o
#     cabecalho do arquivo dele) exercitam o caminho real de I/O de stdout -- nao so o exit code
#     -- e precisam ter a saida verificada byte-a-byte contra um golden file, nao so olhada por
#     cima com `od -c`. Um segundo manifesto, tests/expected_stdout.txt ("<nome> <caminho-do-
#     golden>", mesmo formato de busca do expected_exit.txt), opta IN um programa pra essa
#     checagem mais rigorosa; um programa ausente dele mantem o contrato original de so-exit-code
#     inalterado. Quando existe uma entrada golden, o stdout e' redirecionado pra um arquivo
#     temporario (nunca uma substituicao de comando de shell `$(...)`, que comeria qualquer
#     quebra-de-linha final e corromperia silenciosamente a comparacao byte-a-byte) e comparado
#     com `cmp -s` -- `cmp` POSIX sem saida, exit 0 sse todo byte bate.
test: build
	@status=0; \
	for prog in $(PROGRAMS); do \
		name=$$(basename $$prog); \
		expected=$$(awk -v n="$$name" '$$1==n{print $$2}' tests/expected_exit.txt 2>/dev/null); \
		[ -n "$$expected" ] || expected=0; \
		golden=$$(awk -v n="$$name" '!/^#/ && NF>0 && $$1==n{print $$2}' tests/expected_stdout.txt 2>/dev/null); \
		if [ -n "$$golden" ]; then \
			outtmp=$$(mktemp); \
			$$prog < /dev/null > "$$outtmp"; actual=$$?; \
			if [ "$$actual" = "$$expected" ] && cmp -s "$$outtmp" "$$golden"; then \
				echo "PASS: $$name (exit=$$actual, stdout==$$golden)"; \
			else \
				echo "FAIL: $$name (exit=$$actual, expected=$$expected, stdout vs $$golden mismatch)"; \
				status=1; \
			fi; \
			rm -f "$$outtmp"; \
		else \
			$$prog < /dev/null; actual=$$?; \
			if [ "$$actual" = "$$expected" ]; then \
				echo "PASS: $$name (exit=$$actual)"; \
			else \
				echo "FAIL: $$name (exit=$$actual, expected=$$expected)"; \
				status=1; \
			fi; \
		fi; \
	done; \
	exit $$status

run: build
	@for prog in $(PROGRAMS); do echo "--- $$prog ---"; $$prog; echo "exit=$$?"; done

clean:
	rm -rf $(BUILD)
