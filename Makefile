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
#     increment -- superseded by the C1 test runner (TODO.md, W6) once it exists.
# PT: Roda todo programa e checa o exit code contra o manifesto tests/expected_exit.txt
#     (linhas "<nome> <codigo-esperado>"; entrada ausente assume 0). Harness temporario deste
#     incremento -- substituido pelo runner de teste C1 (TODO.md, W6) quando existir.
test: build
	@status=0; \
	for prog in $(PROGRAMS); do \
		name=$$(basename $$prog); \
		expected=$$(awk -v n="$$name" '$$1==n{print $$2}' tests/expected_exit.txt 2>/dev/null); \
		[ -n "$$expected" ] || expected=0; \
		$$prog; actual=$$?; \
		if [ "$$actual" = "$$expected" ]; then \
			echo "PASS: $$name (exit=$$actual)"; \
		else \
			echo "FAIL: $$name (exit=$$actual, expected=$$expected)"; \
			status=1; \
		fi; \
	done; \
	exit $$status

run: build
	@for prog in $(PROGRAMS); do echo "--- $$prog ---"; $$prog; echo "exit=$$?"; done

clean:
	rm -rf $(BUILD)
