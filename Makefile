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

.PHONY: build test test-negative check-static clean run

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
# EN: TST-INT (TODO.md, W11) extension: a THIRD manifest, tests/expected_stderr.txt, mirrors
#     expected_stdout.txt's exact shape ("<name> <golden-path>") but for fd 2. This is what
#     versions AUD-C0-5.3's negative-path proof: tests/negative_probe.c is a gate program whose
#     job is to FAIL on purpose (TEST_ASSERT with a false condition) and its stderr -- the
#     "FAIL: <file>:<line>: <expr>" message from include/test.h -- is now diffed byte-exact
#     against a committed golden, instead of that proof living only as a throwaway manual probe
#     (see tests/selftest.c's file header for the prior state). A program with neither a stdout
#     nor a stderr golden keeps running with both fds inherited from the invoking shell (so e.g.
#     hello's "hello, world" still shows up live in `make test`'s own output) -- captures only
#     happen for whichever fd(s) actually have a golden registered, to avoid silently swallowing
#     output nobody asked to have checked.
# PT: Extensao do TST-INT (TODO.md, W11): um TERCEIRO manifesto, tests/expected_stderr.txt,
#     espelha exatamente o formato do expected_stdout.txt ("<nome> <caminho-do-golden>") mas pro
#     fd 2. E' isso que versiona a prova do caminho NEGATIVO do AUD-C0-5.3:
#     tests/negative_probe.c e' um programa-gate cujo trabalho e' FALHAR de proposito
#     (TEST_ASSERT com condicao falsa) e o stderr dele -- a mensagem "FAIL: <arquivo>:<linha>:
#     <expr>" do include/test.h -- agora e' comparado byte-a-byte contra um golden commitado, em
#     vez dessa prova viver so como uma sonda manual descartavel (ver o cabecalho do arquivo de
#     tests/selftest.c pro estado anterior). Um programa sem golden nem de stdout nem de stderr
#     continua rodando com os dois fds herdados do shell que chamou (entao, ex., o "hello, world"
#     do hello ainda aparece ao vivo na saida do proprio `make test`) -- so ha captura pro(s)
#     fd(s) que de fato tem golden registrado, pra nao engolir em silencio saida que ninguem
#     pediu pra checar.
test: build
	@status=0; \
	for prog in $(PROGRAMS); do \
		name=$$(basename $$prog); \
		expected=$$(awk -v n="$$name" '$$1==n{print $$2}' tests/expected_exit.txt 2>/dev/null); \
		[ -n "$$expected" ] || expected=0; \
		golden_out=$$(awk -v n="$$name" '!/^#/ && NF>0 && $$1==n{print $$2}' tests/expected_stdout.txt 2>/dev/null); \
		golden_err=$$(awk -v n="$$name" '!/^#/ && NF>0 && $$1==n{print $$2}' tests/expected_stderr.txt 2>/dev/null); \
		if [ -n "$$golden_out" ] || [ -n "$$golden_err" ]; then \
			outtmp=$$(mktemp); errtmp=$$(mktemp); \
			$$prog < /dev/null > "$$outtmp" 2> "$$errtmp"; actual=$$?; \
			ok=1; detail=""; \
			[ "$$actual" = "$$expected" ] || ok=0; \
			if [ -n "$$golden_out" ]; then \
				if cmp -s "$$outtmp" "$$golden_out"; then detail="$$detail stdout==$$golden_out"; else ok=0; detail="$$detail stdout!=$$golden_out"; fi; \
			fi; \
			if [ -n "$$golden_err" ]; then \
				if cmp -s "$$errtmp" "$$golden_err"; then detail="$$detail stderr==$$golden_err"; else ok=0; detail="$$detail stderr!=$$golden_err"; fi; \
			fi; \
			if [ "$$ok" = 1 ]; then \
				echo "PASS: $$name (exit=$$actual,$$detail)"; \
			else \
				echo "FAIL: $$name (exit=$$actual, expected=$$expected,$$detail)"; \
				status=1; \
			fi; \
			rm -f "$$outtmp" "$$errtmp"; \
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

# EN: AUD-C0-5.3 convenience alias: re-runs JUST negative_probe (already part of the loop above
#     via tests/expected_exit.txt + tests/expected_stderr.txt) with a dedicated, narrowly-scoped
#     target -- useful for fast local iteration on the harness itself (include/test.h) without
#     rebuilding/rerunning the whole suite, and as an explicitly-named CI/pre-commit check. Not a
#     SEPARATE code path from `test`'s generic stderr-golden logic above -- same manifests, same
#     comparison, just filtered to one program.
# PT: Alias de conveniencia do AUD-C0-5.3: reroda SO o negative_probe (ja parte do laco acima via
#     tests/expected_exit.txt + tests/expected_stderr.txt) com um alvo dedicado e de escopo
#     estreito -- util pra iteracao local rapida no proprio harness (include/test.h) sem
#     rebuildar/rerodar a suite inteira, e como uma checagem de CI/pre-commit explicitamente
#     nomeada. Nao e' um caminho de codigo SEPARADO da logica generica de golden-de-stderr do
#     `test` acima -- mesmos manifestos, mesma comparacao, so filtrado pra um programa.
test-negative: $(BIN)/negative_probe
	@name=negative_probe; \
	prog=$(BIN)/$$name; \
	expected=$$(awk -v n="$$name" '$$1==n{print $$2}' tests/expected_exit.txt); \
	golden_err=$$(awk -v n="$$name" '!/^#/ && NF>0 && $$1==n{print $$2}' tests/expected_stderr.txt); \
	if [ -z "$$golden_err" ]; then \
		echo "test-negative: FAILED ($$name has no entry in tests/expected_stderr.txt)"; \
		exit 1; \
	fi; \
	errtmp=$$(mktemp); \
	$$prog < /dev/null > /dev/null 2> "$$errtmp"; actual=$$?; \
	if [ "$$actual" = "$$expected" ] && cmp -s "$$errtmp" "$$golden_err"; then \
		echo "PASS: $$name (exit=$$actual, stderr==$$golden_err) -- AUD-C0-5.3 TEST_ASSERT failure path verified"; \
		rm -f "$$errtmp"; \
	else \
		echo "FAIL: $$name (exit=$$actual, expected=$$expected, stderr vs $$golden_err mismatch)"; \
		rm -f "$$errtmp"; \
		exit 1; \
	fi

run: build
	@for prog in $(PROGRAMS); do echo "--- $$prog ---"; $$prog; echo "exit=$$?"; done

# EN: TST-STATIC (TODO.md, W11): static analysis without executing anything. Three checks,
#     scoped to src/ (the runtime/"libc" itself -- see TESTES.md for why tests/*.c is out of
#     scope for cppcheck here: those files deliberately exercise OUR OWN string/mem functions
#     with patterns cppcheck's libc-shaped heuristics misread as noise, e.g. "unnecessary
#     comparison of static strings" for a test that is precisely proving strcmp()'s behaviour):
#       (a) cppcheck in freestanding mode (no missingIncludeSystem noise -- there IS no libc to
#           find headers for, that "gap" is the whole point of this codebase);
#       (b) clang --analyze with the EXACT same freestanding flags as the real build (CFLAGS,
#           minus warnings/paths that do not apply to single-TU analysis) -- catches UB/misuse
#           classes cppcheck does not (e.g. deeper interprocedural null-deref paths);
#       (c) tools/check_spdx.sh (STD-SPDX, TODO.md) -- every code file carries the license header;
#       (d) tools/check_syscall_nums_sync.sh (AUD-C0 finding) -- the C/NASM syscall-number
#           mirrors have not drifted apart.
#     Any cppcheck/clang-analyzer finding that is real gets fixed in src/ directly; a finding
#     that is an inevitable false positive in this freestanding context is suppressed
#     CIRURGICALLY at its exact call site (`// cppcheck-suppress <id>` with a rationale comment
#     immediately above -- see src/sys_mmap.c / src/sys_read.c for two real examples), never by
#     disabling the whole check class.
# PT: TST-STATIC (TODO.md, W11): analise estatica sem executar nada. Tres/quatro checagens,
#     restritas a src/ (a runtime/"libc" em si -- ver TESTES.md pro motivo de tests/*.c estar
#     fora de escopo do cppcheck aqui: esses arquivos exercitam DE PROPOSITO nossas PROPRIAS
#     funcoes de string/memoria com padroes que as heuristicas do cppcheck, moldadas pra libc,
#     leem errado como ruido, ex.: "comparacao desnecessaria de strings estaticas" pra um teste
#     que esta precisamente provando o comportamento do nosso strcmp()):
#       (a) cppcheck em modo freestanding (sem ruido de missingIncludeSystem -- NAO HA libc pra
#           achar headers, essa "lacuna" e' o ponto inteiro deste codigo-base);
#       (b) clang --analyze com as MESMAS flags freestanding do build real (CFLAGS, menos
#           avisos/caminhos que nao se aplicam a analise de uma unica TU) -- pega classes de
#           UB/uso indevido que o cppcheck nao pega (ex.: caminhos de null-deref interprocedurais
#           mais profundos);
#       (c) tools/check_spdx.sh (STD-SPDX, TODO.md) -- todo arquivo de codigo carrega o header
#           de licenca;
#       (d) tools/check_syscall_nums_sync.sh (achado AUD-C0) -- os espelhos C/NASM dos numeros
#           de syscall nao driftaram um do outro.
#     Todo achado real de cppcheck/clang-analyzer e' corrigido direto em src/; um achado que e'
#     falso-positivo inevitavel neste contexto freestanding e' suprimido CIRURGICAMENTE no
#     ponto de chamada exato (`// cppcheck-suppress <id>` com comentario de racional logo acima
#     -- ver src/sys_mmap.c / src/sys_read.c pra dois exemplos reais), nunca desligando a
#     checagem inteira.
check-static:
	@echo "--- (a) cppcheck (freestanding, src/) ---"
	cppcheck --enable=warning,style,performance,portability --std=c23 --platform=unix64 \
	         --suppress=missingIncludeSystem --inline-suppr --error-exitcode=2 -Iinclude src/
	@echo "--- (b) clang --analyze (freestanding, src/*.c) ---"
	@status=0; \
	for f in src/*.c; do \
		out=$$(clang --analyze -Xclang -analyzer-output=text $(CFLAGS) "$$f" -o /dev/null 2>&1); \
		if [ -n "$$out" ]; then echo "$$out"; status=1; fi; \
	done; \
	if [ "$$status" -eq 0 ]; then echo "clang --analyze: OK (0 findings across src/*.c)"; else exit 1; fi
	@echo "--- (c) SPDX header presence ---"
	tools/check_spdx.sh
	@echo "--- (d) syscall_nums.h <-> syscall_nums.inc drift ---"
	tools/check_syscall_nums_sync.sh

clean:
	rm -rf $(BUILD)
