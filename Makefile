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

CC  := clang
AS  := nasm
LD  := ld
AR  := ar
CXX := clang++

CFLAGS  := -std=c23 -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
           -fno-builtin -fno-asynchronous-unwind-tables -m64 -Wall -Wextra -Iinclude
ASFLAGS := -f elf64 -Iinclude/
LDFLAGS := -nostdlib -static -no-pie -e _start

# EN: SOV-LIBCORE (ADR-0009, FT-F0) -- HOSTED C++ flags for tests/libcore_consumer.cpp, the
#     ordinary g++/clang++ program that links build/libcore.a next to a normal libc/libstdc++
#     (the opposite of every other flag block on this page: no -ffreestanding, no -nostdlib --
#     this is deliberately a REAL hosted build, proving libcore.a is consumable from one). See
#     src/core_api.c's file header for the archive-side rename mechanism this build proves out.
# PT: SOV-LIBCORE (ADR-0009, FT-F0) -- flags C++ HOSTED pro tests/libcore_consumer.cpp, o
#     programa g++/clang++ comum que linka a build/libcore.a ao lado de uma libc/libstdc++
#     normal (o oposto de todo outro bloco de flag desta página: sem -ffreestanding, sem
#     -nostdlib -- este é deliberadamente um build hosted DE VERDADE, provando que a
#     libcore.a é consumível a partir de um). Ver o cabeçalho de arquivo do src/core_api.c pro
#     mecanismo de rename do lado do archive que este build prova.
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude

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
# EN: `tests/sanitize_sfnt.c` is EXCLUDED from this wildcard on purpose -- unlike every other
#     tests/*.c file, it is a plain HOSTED (libc-linked) program, not a freestanding gate
#     program (see that file's own header) -- it would fail to link against $(RUNTIME_OBJS)
#     (`-nostdlib`, no libc `fopen`/`fprintf`/etc.) if swept in here. Built/run by its own
#     dedicated `sanitize-sfnt` target below instead (same pattern `tests/libcore_consumer.cpp`
#     already uses, just via extension-based exclusion there instead of an explicit filter-out).
# PT: `tests/sanitize_sfnt.c` é EXCLUÍDO deste wildcard de propósito -- diferente de todo outro
#     tests/*.c, é um programa HOSTED comum (linkado com libc), não um programa-gate freestanding
#     (ver o cabeçalho próprio daquele arquivo) -- falharia ao linkar contra $(RUNTIME_OBJS)
#     (`-nostdlib`, sem `fopen`/`fprintf`/etc. da libc) se fosse varrido aqui pra dentro. Buildado/
#     rodado pelo próprio alvo dedicado `sanitize-sfnt` abaixo em vez disso (mesmo padrão que o
#     `tests/libcore_consumer.cpp` já usa, só que via exclusão baseada em extensão lá em vez de um
#     filter-out explícito).
PROGRAM_SRCS := $(filter-out tests/sanitize_sfnt.c,$(wildcard tests/*.c))
PROGRAMS     := $(patsubst tests/%.c,$(BIN)/%,$(PROGRAM_SRCS))

.PHONY: build test test-negative check-static test-mem clean run libcore libcore-test sanitize-sfnt

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

# EN: SOV-SFNT (FT-F1) test fixture -- tests/test_sfnt.c needs a REAL TrueType font blob to
#     parse (the Open Sans already used by glintfx's showcase demo, duplicated here at
#     tests/fixtures/opensans_regular.ttf so Layer 0 does not reach across into glintfx/'s tree
#     -- the two layers stay independently buildable/testable). Layer 0 has no `open`/`openat`
#     syscall wrapper (out of this ticket's scope -- glx_sfnt_open is a pure blob-in parser, file
#     I/O is the future L1.19-FONTENG C++ shell's job, see include/core/sfnt.h's file header), so
#     the fixture cannot be `read()` off disk at runtime the normal way. Instead it is embedded
#     at LINK TIME via the classic `ld -r -b binary` trick: this turns the raw font bytes into a
#     relocatable object exposing `_binary_opensans_regular_ttf_start`/`_end` symbols (verified
#     empirically for this exact GNU ld -- see task report) -- zero libc, zero syscalls, the
#     bytes simply live in the final binary's .data the same way a `static const` array would,
#     without needing ~1MB of generated `0x..,` source text for a 217KB binary. `cd`ing into
#     tests/fixtures/ first (rather than passing the path directly) keeps the exported symbol
#     name clean (`ld -b binary`'s symbol name is derived from the exact argument string --
#     passing a path with directory components would bake `tests_fixtures_` into the symbol);
#     $(abspath ...) is expanded by `make` BEFORE the recipe's `cd` runs, so the output path
#     stays correct despite the working-directory change.
# PT: Fixture de teste do SOV-SFNT (FT-F1) -- tests/test_sfnt.c precisa de um blob de fonte
#     TrueType DE VERDADE pra parsear (a mesma Open Sans já usada pelo demo showcase do
#     glintfx, duplicada aqui em tests/fixtures/opensans_regular.ttf pra Camada 0 não alcançar
#     pra dentro da árvore do glintfx/ -- as duas camadas continuam buildáveis/testáveis
#     independentemente). A Camada 0 não tem wrapper de syscall `open`/`openat` (fora do escopo
#     desta tarefa -- glx_sfnt_open é um parser puro blob-entra, I/O de arquivo é trabalho da
#     futura casca C++ L1.19-FONTENG, ver o cabeçalho de arquivo do include/core/sfnt.h), então
#     a fixture não pode ser `read()`ada do disco em runtime do jeito normal. Em vez disso é
#     embutida em TEMPO DE LINK via o truque clássico `ld -r -b binary`: isso transforma os
#     bytes crus da fonte num objeto relocável expondo símbolos
#     `_binary_opensans_regular_ttf_start`/`_end` (verificado empiricamente pra este `ld` GNU
#     exato -- ver relatório da tarefa) -- zero libc, zero syscalls, os bytes simplesmente moram
#     no .data do binário final do mesmo jeito que um array `static const` moraria, sem precisar
#     de ~1MB de texto-fonte gerado `0x..,` pra um binário de 217KB. Entrar (`cd`) em
#     tests/fixtures/ primeiro (em vez de passar o caminho direto) mantém o nome de símbolo
#     exportado limpo (o nome de símbolo do `ld -b binary` é derivado da string exata do
#     argumento -- passar um caminho com componentes de diretório assaria `tests_fixtures_` no
#     símbolo); $(abspath ...) é expandido pelo `make` ANTES do `cd` da receita rodar, então o
#     caminho de saída continua correto apesar da mudança de diretório de trabalho.
$(OBJ)/opensans_ttf.o: tests/fixtures/opensans_regular.ttf | $(OBJ)
	cd tests/fixtures && $(LD) -r -b binary -o $(abspath $(OBJ))/opensans_ttf.o opensans_regular.ttf

# EN: test_sfnt needs the fixture object above IN ADDITION to $(RUNTIME_OBJS) -- an explicit,
#     target-specific rule overrides the generic `$(BIN)/%:` pattern rule above for this ONE
#     target (standard GNU Make precedence: an explicit rule for a target is always preferred
#     over a pattern rule, no `%` matching is even attempted once one exists).
# PT: O test_sfnt precisa do objeto de fixture acima ALÉM de $(RUNTIME_OBJS) -- uma regra
#     explícita, específica do alvo, sobrepõe a regra de padrão genérica `$(BIN)/%:` acima pra
#     este ÚNICO alvo (precedência padrão do GNU Make: uma regra explícita pra um alvo é sempre
#     preferida sobre uma regra de padrão, nenhum casamento de `%` sequer é tentado uma vez que
#     uma já existe).
$(BIN)/test_sfnt: $(OBJ)/test_sfnt.o $(RUNTIME_OBJS) $(OBJ)/opensans_ttf.o | $(BIN)
	$(LD) $(LDFLAGS) -o $@ $^

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

# EN: TST-MEM (TODO.md, W11): memory validation of the E1 allocator. Runs `valgrind --tool=
#     memcheck` over the two gate programs that most directly exercise raw memory (test_alloc,
#     the allocator's own suite, and test_mem, the D1 memcpy/memset/memmove/memcmp primitives
#     the allocator is built on).
#
#     DOCUMENTED LIMITATION (verified empirically, not assumed -- see task report): valgrind
#     DOES successfully instrument this freestanding/no-libc/static/no-PIE binary (confirmed: it
#     ran clean, and separately, on an intentionally-corrupted probe that wrote 2 MiB past a
#     16-byte allocation, it correctly reported "Invalid write of size 1 ... not stack'd,
#     malloc'd or (recently) free'd" before the SIGSEGV). What it CANNOT do here is anything
#     malloc/free-hook-shaped: every run's "HEAP SUMMARY" reports "0 allocs, 0 frees, 0 bytes
#     allocated" because src/alloc.c never calls libc's malloc/free (there is no libc) -- it
#     bump-allocates directly over raw `mmap` syscalls (src/sys_mmap.c), which valgrind does not
#     attribute to its malloc-shaped heap accounting. Concretely this means valgrind here catches
#     GROSS corruption (writes into entirely unmapped address space -- a real signal, see above)
#     but NOT the two things "TST-MEM" nominally promises and a libc-hosted valgrind run would
#     catch for free: (1) leaks -- moot by construction anyway, this project's free() is a
#     documented no-op / leak-by-design (ADR-0004, ADR-0004's realloc/free contract) so "no
#     leaks" is not a meaningful pass/fail signal; (2) an out-of-bounds write that lands INSIDE
#     the current 1 MiB bump arena but past the end of the specific block that was handed out --
#     invisible to valgrind because our allocator has no per-block redzone (only a real malloc
#     hook gets that instrumentation for free).
#
#     FALLBACK (what actually covers those two gaps): tests/test_alloc.c's own assertions --
#     every allocation's 16-byte alignment is checked explicitly (TEST_ASSERT_EQ(...% 16, 0)),
#     and the "multiple mallocs: distinct, non-overlapping" + arena-rollover cases stamp a UNIQUE
#     byte pattern into each block and re-verify it after allocating siblings, which is exactly
#     the invariant check (byte-accounting via harness asserts) a bump allocator without
#     redzones can meaningfully offer -- if the allocator mis-sized/mis-aligned/overlapped a
#     block, one of those writes would have clobbered a neighbour and TEST_ASSERT_EQ would abort
#     with FAIL: file:line (verified live by `make test`, which this target also depends on
#     transitively via `build`). This target does not fake a clean bill of health it cannot
#     produce; it reports the real, narrower guarantee valgrind gives here (no gross corruption)
#     on top of that harness-level coverage.
# PT: TST-MEM (TODO.md, W11): validacao de memoria do alocador E1. Roda `valgrind --tool=
#     memcheck` sobre os dois programas-gate que mais diretamente exercitam memoria crua
#     (test_alloc, a suite propria do alocador, e test_mem, as primitivas D1
#     memcpy/memset/memmove/memcmp sobre as quais o alocador e' construido).
#
#     LIMITACAO DOCUMENTADA (verificada empiricamente, nao suposta -- ver relatorio da tarefa): o
#     valgrind CONSEGUE instrumentar com sucesso este binario freestanding/sem-libc/estatico/
#     no-PIE (confirmado: rodou limpo, e separadamente, numa sonda intencionalmente corrompida
#     que escreveu 2 MiB alem de uma alocacao de 16 bytes, reportou corretamente "Invalid write
#     of size 1 ... not stack'd, malloc'd or (recently) free'd" antes do SIGSEGV). O que ele NAO
#     CONSEGUE fazer aqui e' qualquer coisa moldada em hook de malloc/free: todo "HEAP SUMMARY"
#     reporta "0 allocs, 0 frees, 0 bytes allocated" porque src/alloc.c nunca chama malloc/free
#     da libc (nao ha libc) -- ele bump-aloca direto sobre syscalls `mmap` cruas (src/sys_mmap.c),
#     que o valgrind nao atribui a sua contabilidade de heap moldada em malloc. Concretamente
#     isso significa que o valgrind aqui pega corrupcao GROSSEIRA (escritas em espaco de
#     endereco totalmente nao-mapeado -- um sinal real, ver acima) mas NAO as duas coisas que o
#     "TST-MEM" nominalmente promete e que uma corrida de valgrind hospedada em libc pegaria de
#     graca: (1) leaks -- discutivel por construcao de qualquer jeito, o free() deste projeto e'
#     um no-op documentado / leak-by-design (ADR-0004, contrato realloc/free da ADR-0004) entao
#     "sem leaks" nao e' um sinal significativo de passa/falha; (2) uma escrita fora dos limites
#     que caia DENTRO da arena-bump de 1 MiB atual mas alem do fim do bloco especifico que foi
#     entregue -- invisivel pro valgrind porque nosso alocador nao tem redzone por-bloco (so um
#     hook de malloc de verdade ganha essa instrumentacao de graca).
#
#     FALLBACK (o que de fato cobre essas duas lacunas): as proprias assercoes de
#     tests/test_alloc.c -- todo alinhamento de 16 bytes de alocacao e' checado explicitamente
#     (TEST_ASSERT_EQ(...% 16, 0)), e os casos "multiplos mallocs: distintos, nao-sobrepostos" +
#     rollover-de-arena carimbam um padrao de byte UNICO em cada bloco e reconferem depois de
#     alocar irmaos, que e' exatamente a checagem de invariante (contabilidade de bytes via
#     asserts do harness) que um bump allocator sem redzone consegue oferecer de forma
#     significativa -- se o alocador dimensionasse/alinhasse/sobrepusesse mal um bloco, uma
#     dessas escritas teria atropelado um vizinho e o TEST_ASSERT_EQ abortaria com FAIL:
#     arquivo:linha (verificado ao vivo pelo `make test`, do qual este alvo tambem depende
#     transitivamente via `build`). Este alvo nao finge uma certidao de saude limpa que nao pode
#     produzir; reporta a garantia real, mais estreita, que o valgrind da aqui (sem corrupcao
#     grosseira) em cima dessa cobertura em nivel de harness.
test-mem: build
	@echo "--- valgrind memcheck: test_alloc (E1 allocator suite) ---"
	valgrind --error-exitcode=97 --leak-check=full $(BIN)/test_alloc < /dev/null
	@echo "--- valgrind memcheck: test_mem (D1 memcpy/memset/memmove/memcmp primitives) ---"
	valgrind --error-exitcode=97 --leak-check=full $(BIN)/test_mem < /dev/null
	@echo "test-mem: OK (no gross memory corruption under valgrind; see Makefile comment above for the documented malloc-hook-tracking limitation of this freestanding target -- real invariant coverage is tests/test_alloc.c's alignment/non-overlap asserts, exercised by 'make test')"

# EN: SOV-LIBCORE (ADR-0009, FT-F0): the PHYSICAL boundary the internalization ADR calls for --
#     a static archive, build/libcore.a, whose EXPORTED symbols carry the `glx_` prefix, safe to
#     link into a hosted (non-freestanding) C++ consumer sitting next to a normal libc/libstdc++.
#     See include/core/core.h (the public surface) and src/core_api.c (the rename mechanism that
#     makes this safe) for the full design. `L1.14-GLLOADER` did NOT exercise this archive (the
#     gl3w-piloto lived entirely inside glintfx) -- this is the first piece to actually prove the
#     `.a` + `glx_` + hosted-consumption pattern end to end.
#
#     TWO OBJECT TREES, ONE SOURCE TREE: `build/obj/` (the existing freestanding tree, unchanged)
#     keeps building every src/*.c file the NORMAL way (no renaming) for every tests/*.c gate
#     program via $(RUNTIME_OBJS) -- `make build`/`make test` are 100% unaffected by anything
#     below. `build/objcore/` is a SECOND, ARCHIVE-ONLY compilation of the SAME src/*.c files,
#     with $(CORE_RENAME_FLAGS) applied uniformly (see src/core_api.c's file header for exactly
#     why and how).
#
#     SECURITY-ENGINEER FINDING (post-decb2fb review, CRITICAL, fixed here): the FIRST cut of
#     $(CORE_RENAME_FLAGS) only covered the 7 malloc/free/realloc/mem* names -- but `wildcard
#     src/*.c` archives EVERY src/*.c file, including str.c/conv.c/printf.c, whose libc-shaped
#     names (`strlen`/`strcmp`/... /`atoi`/`atof`/... /`mini_printf`/`mini_vsnprintf`) were left
#     BARE in build/libcore.a -- a hosted C++ consumer linking the archive would silently get
#     THIS project's `strlen`/`atoi`/etc. instead of glibc's for the WHOLE process (symbol
#     hijack), proven live. $(CORE_RENAME_FLAGS) below now covers the FULL family of libc-shaped
#     identifiers present in src/*.c today (renamed to `glx_core_<name>_impl`, uniformly, the
#     same mechanism as the original 7) -- chosen over the brief's alternative ("restrict
#     CORE_C_OBJS to only what core_api.c references today") because the FreeType epic (AUD-
#     TRANS-2) will transitively pull str/printf into the archive anyway, so the wider rename is
#     the future-proof fix, not a narrower one that would need re-litigating per epic.
#     `alloc_owns_ptr` (src/alloc.c, include/alloc_internal.h) is renamed too now -- it was never
#     a colliding libc name, but it is an internal helper with no business being a bare exported
#     symbol either; folding it into the same `glx_core_*_impl` family is simpler than a bespoke
#     whitelist entry (see tools/check_libcore_symbols.sh). `tools/check_libcore_symbols.sh` (gate
#     4, `make libcore-test`) enforces this exhaustively now: a POSITIVE whitelist of every symbol
#     name this archive is allowed to export, not a blacklist of the 7 names that bit us -- ANY
#     other defined global symbol (a str/conv/printf name added tomorrow and forgotten here, for
#     instance) fails the build instead of silently passing.
#     The two `.asm` runtime objects (start.o/syscall.o) are REUSED as-is from build/obj/ -- no
#     C preprocessor touches NASM source, and neither `_start` nor `syscall1..6` collide with any
#     libc name to begin with (see src/core_api.c's header for the full rationale on why only
#     the C primitives needed renaming).
# PT: SOV-LIBCORE (ADR-0009, FT-F0): a fronteira FÍSICA que a ADR de internalização pede -- um
#     archive estático, build/libcore.a, cujos símbolos EXPORTADOS carregam o prefixo `glx_`,
#     seguro de linkar num consumidor C++ hosted (não-freestanding) sentado ao lado de uma
#     libc/libstdc++ normal. Ver include/core/core.h (a superfície pública) e src/core_api.c (o
#     mecanismo de rename que torna isso seguro) pro desenho completo. O `L1.14-GLLOADER` NÃO
#     exercitou este archive (o gl3w-piloto morava inteiramente dentro do glintfx) -- esta é a
#     primeira peça a de fato provar o padrão `.a` + `glx_` + consumo-hosted de ponta a ponta.
#
#     DUAS ÁRVORES DE OBJETO, UMA ÁRVORE DE FONTE: `build/obj/` (a árvore freestanding existente,
#     intocada) continua compilando todo src/*.c do jeito NORMAL (sem rename) pra todo
#     programa-gate tests/*.c via $(RUNTIME_OBJS) -- `make build`/`make test` ficam 100%
#     não-afetados por qualquer coisa abaixo. `build/objcore/` é uma SEGUNDA compilação, SÓ PRO
#     ARCHIVE, dos MESMOS src/*.c, com $(CORE_RENAME_FLAGS) aplicado uniformemente (ver o
#     cabeçalho de arquivo do src/core_api.c pro porquê e como exatos).
#
#     ACHADO DO SECURITY-ENGINEER (revisão pós-decb2fb, CRÍTICO, corrigido aqui): o PRIMEIRO corte
#     do $(CORE_RENAME_FLAGS) só cobria os 7 nomes malloc/free/realloc/mem* -- mas o `wildcard
#     src/*.c` arquiva TODO src/*.c, inclusive str.c/conv.c/printf.c, cujos nomes no formato de
#     libc (`strlen`/`strcmp`/... /`atoi`/`atof`/... /`mini_printf`/`mini_vsnprintf`) ficavam CRUS
#     na build/libcore.a -- um consumidor C++ hosted linkando o archive receberia em silêncio o
#     `strlen`/`atoi`/etc. DESTE projeto em vez do da glibc pro processo INTEIRO (sequestro de
#     símbolo), provado ao vivo. O $(CORE_RENAME_FLAGS) abaixo agora cobre a família COMPLETA de
#     identificadores no formato de libc presente em src/*.c hoje (renomeados pra
#     `glx_core_<nome>_impl`, uniformemente, mesmo mecanismo dos 7 originais) -- escolhido em vez
#     da alternativa do brief ("restringir CORE_C_OBJS só ao que o core_api.c referencia hoje")
#     porque o épico FreeType (AUD-TRANS-2) vai puxar str/printf pro archive transitivamente de
#     qualquer jeito, então o rename mais amplo é o fix à prova de futuro, não um mais estreito
#     que precisaria ser relitigado por épico.
#     O `alloc_owns_ptr` (src/alloc.c, include/alloc_internal.h) também é renomeado agora -- nunca
#     foi um nome colidente de libc, mas é um helper interno sem motivo pra ser um símbolo cru
#     exportado tampouco; dobrar ele na mesma família `glx_core_*_impl` é mais simples que uma
#     entrada de whitelist sob-medida (ver tools/check_libcore_symbols.sh). O
#     `tools/check_libcore_symbols.sh` (gate 4, `make libcore-test`) aplica isso exaustivamente
#     agora: uma whitelist POSITIVA de todo nome de símbolo que este archive tem permissão de
#     exportar, não uma lista negra dos 7 nomes que nos morderam -- QUALQUER outro símbolo global
#     definido (um nome de str/conv/printf acrescentado amanhã e esquecido aqui, por exemplo)
#     falha o build em vez de passar em silêncio.
#     Os dois objetos de runtime `.asm` (start.o/syscall.o) são REUSADOS como estão de build/obj/
#     -- nenhum pré-processador C toca fonte NASM, e nem `_start` nem `syscall1..6` colidem com
#     nome nenhum de libc pra começo de conversa (ver o cabeçalho do src/core_api.c pro racional
#     completo de por que só as primitivas C precisaram de rename).
OBJCORE := $(BUILD)/objcore

CORE_RENAME_FLAGS := -Dmalloc=glx_core_malloc_impl -Dfree=glx_core_free_impl \
                      -Drealloc=glx_core_realloc_impl -Dmemcpy=glx_core_memcpy_impl \
                      -Dmemset=glx_core_memset_impl -Dmemmove=glx_core_memmove_impl \
                      -Dmemcmp=glx_core_memcmp_impl -Dalloc_owns_ptr=glx_core_alloc_owns_ptr_impl \
                      -Dstrlen=glx_core_strlen_impl -Dstrcmp=glx_core_strcmp_impl \
                      -Dstrncmp=glx_core_strncmp_impl -Dstrcpy=glx_core_strcpy_impl \
                      -Dstrncpy=glx_core_strncpy_impl -Dstrcat=glx_core_strcat_impl \
                      -Dstrchr=glx_core_strchr_impl -Datoi=glx_core_atoi_impl \
                      -Datof=glx_core_atof_impl -Datou=glx_core_atou_impl \
                      -Ditoa=glx_core_itoa_impl -Dutoa=glx_core_utoa_impl \
                      -Dftoa=glx_core_ftoa_impl -Dmini_printf=glx_core_mini_printf_impl \
                      -Dmini_vsnprintf=glx_core_mini_vsnprintf_impl

CORE_C_OBJS   := $(patsubst src/%.c,$(OBJCORE)/%.o,$(RUNTIME_C_SRCS))
CORE_ASM_OBJS := $(patsubst src/%.asm,$(OBJ)/%.o,$(RUNTIME_ASM_SRCS))
CORE_OBJS     := $(CORE_C_OBJS) $(CORE_ASM_OBJS)

$(OBJCORE)/%.o: src/%.c | $(OBJCORE)
	$(CC) $(CFLAGS) $(CORE_RENAME_FLAGS) -c $< -o $@

$(OBJCORE):
	mkdir -p $@

libcore: $(BUILD)/libcore.a

$(BUILD)/libcore.a: $(CORE_OBJS) | $(BUILD)
	$(AR) rcs $@ $(CORE_OBJS)

# EN: SOV-LIBCORE gate 3/4 verification: builds tests/libcore_consumer.cpp (a HOSTED, ordinary
#     C++ program -- NOT a freestanding gate program, NOT part of $(PROGRAMS)/`make test`) that
#     links against build/libcore.a next to real libc/libstdc++, exercises correct glx_malloc/
#     glx_free/glx_realloc/glx_memcpy/glx_memset usage AND deliberately misuses the heap-
#     ownership boundary (forked child processes, expected to die loudly -- see that file's own
#     header, including the alignment-misuse case 3c added by the security-engineer fix), then
#     runs tools/check_libcore_symbols.sh (gate 4: `nm -g` against a POSITIVE whitelist proves the
#     ONLY exported names are the glx_* front door + the renamed glx_core_*_impl internals + the
#     expected ASM/wrapper set -- see that script's own header and $(CORE_RENAME_FLAGS)'s comment
#     above for the CRITICAL finding this replaced the old 7-name blacklist for).
# PT: Verificação dos gates 3/4 do SOV-LIBCORE: builda tests/libcore_consumer.cpp (um programa
#     C++ HOSTED comum -- NÃO um programa-gate freestanding, NÃO parte de $(PROGRAMS)/`make
#     test`) que linka contra build/libcore.a ao lado de libc/libstdc++ de verdade, exercita uso
#     correto de glx_malloc/glx_free/glx_realloc/glx_memcpy/glx_memset E mau-usa de propósito a
#     fronteira de ownership de heap (processos filho via fork, esperados morrer alto -- ver o
#     cabeçalho próprio daquele arquivo, inclusive o caso 3c de mau-uso de alinhamento acrescentado
#     pelo fix do security-engineer), depois roda tools/check_libcore_symbols.sh (gate 4: `nm -g`
#     contra uma whitelist POSITIVA prova que os ÚNICOS nomes exportados são a porta-da-frente
#     glx_* + os internos renomeados glx_core_*_impl + o conjunto ASM/wrapper esperado -- ver o
#     cabeçalho próprio daquele script e o comentário do $(CORE_RENAME_FLAGS) acima pro achado
#     CRÍTICO que substituiu a lista negra de 7 nomes antiga por isso).
$(BIN)/libcore_consumer: tests/libcore_consumer.cpp $(BUILD)/libcore.a | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ tests/libcore_consumer.cpp $(BUILD)/libcore.a

libcore-test: $(BIN)/libcore_consumer
	@echo "--- libcore_consumer (hosted C++, links build/libcore.a) ---"
	$(BIN)/libcore_consumer
	@echo "--- tools/check_libcore_symbols.sh (ADR-0009 gate 4: nm -g, positive whitelist) ---"
	tools/check_libcore_symbols.sh

# EN: SOV-SFNT (FT-F1) review-adversarial DEFENSE-IN-DEPTH target: `tests/sanitize_sfnt.c` is a
#     plain HOSTED (NOT freestanding) C program -- links `src/sfnt.c` (UNCHANGED, no #ifdef, the
#     exact same TU `make build`/`make test` already compile freestanding) against ordinary
#     libc, so it and only it can be built with `-fsanitize=address,undefined`. This is what
#     caught the CRITICAL float->int16 UB in `apply_component_transform` (composite glyph
#     transform) that `make test`'s freestanding, non-sanitized harness could not see -- see that
#     file's own header for the full rationale, and TESTES.md for this target's place in the
#     overall test manual. `-O1` (not `-O0`): UBSan's out-of-range-float-to-int check needs the
#     cast to actually execute as written, and `-O1` is the documented sweet spot for ASan/UBSan
#     (fast enough for a CI gate, does not optimize away the very code path being sanitized).
# PT: Alvo de DEFESA-EM-PROFUNDIDADE do review adversarial do SOV-SFNT (FT-F1):
#     `tests/sanitize_sfnt.c` é um programa C HOSTED comum (NÃO freestanding) -- linka o
#     `src/sfnt.c` (INALTERADO, sem #ifdef, a mesma TU exata que `make build`/`make test` já
#     compilam freestanding) contra a libc normal, então ele, e só ele, pode ser buildado com
#     `-fsanitize=address,undefined`. Foi isso que pegou o UB CRÍTICO de float->int16 no
#     `apply_component_transform` (transformação de glyph composto) que o harness freestanding
#     não-sanitizado do `make test` não conseguia ver -- ver o cabeçalho próprio daquele arquivo
#     pro racional completo, e TESTES.md pro lugar deste alvo no manual de teste geral. `-O1` (não
#     `-O0`): a checagem de float-fora-de-faixa-pra-int do UBSan precisa que o cast de fato
#     execute como escrito, e `-O1` é o ponto-doce documentado pra ASan/UBSan (rápido o bastante
#     pra um gate de CI, não otimiza pra fora o próprio caminho de código sendo sanitizado).
SANITIZE_SFNT_FLAGS := -std=c23 -Wall -Wextra -Iinclude -fsanitize=address,undefined \
                       -fno-sanitize-recover=all -g -O1

sanitize-sfnt: | $(BIN)
	$(CC) $(SANITIZE_SFNT_FLAGS) -o $(BIN)/sanitize_sfnt tests/sanitize_sfnt.c src/sfnt.c
	$(BIN)/sanitize_sfnt

clean:
	rm -rf $(BUILD)
