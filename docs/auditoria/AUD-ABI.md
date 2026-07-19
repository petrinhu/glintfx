# AUD-ABI — ABI / ASM Conformance Audit / Auditoria de Conformidade ABI/ASM

- **Date / Data:** 2026-07-09
- **Auditor:** embedded-firmware-engineer (read-only audit; no product code touched)
- **Scope item / Item da TODO.md:** `AUD-ABI` (W12), prerequisites `B3, B4, D1, TST-INT` all delivered/green.
- **Verdict / Veredito:** **CONFORME — no CRITICAL or IMPORTANT findings.** The hand-written ASM (`src/start.asm`, `src/syscall.asm`) and every C↔ASM boundary (`src/sys_exit.c`, `src/sys_write.c`, `src/sys_read.c`, `src/sys_mmap.c`) match the strict System V AMD64 ABI as ratified in ADR-0001/0003/0005, both at the source level and in the actual `clang`/`nasm`/`ld`-generated machine code (verified via `objdump -d -M intel` and `readelf`). 1 COSMETIC observation only.

## Scope / Escopo

**EN:** Per `AUDITORIAS.md` (`AUD-ABI` row) and the task brief: (1) System V AMD64 calling convention — C-ABI arg registers vs the raw `syscall` instruction's own convention (`r10` replacing `rcx`, the 4th arg); (2) callee-saved register preservation (`rbx, rbp, r12-r15`) across all hand-written leaf functions, plus correct handling of the `syscall` instruction's own clobber of `rcx`/`r11`; (3) 16-byte stack alignment at `_start` before the first `call`; (4) ELF layout (static, non-PIE, `_start` entry, no libc/crt); (5) ABI-adjacent UB — red-zone misuse, direction-flag assumptions.

**PT:** Conforme `AUDITORIAS.md` (linha `AUD-ABI`) e o brief da tarefa: (1) convenção de chamada System V AMD64 — registradores de argumento da C-ABI vs a convenção própria da instrução `syscall` crua (`r10` no lugar de `rcx`, o 4º argumento); (2) preservação de registradores callee-saved (`rbx, rbp, r12-r15`) em todas as funções ASM escritas à mão, mais o tratamento correto do clobber que a própria instrução `syscall` faz em `rcx`/`r11`; (3) alinhamento de stack em 16 bytes no `_start` antes da 1ª `call`; (4) layout ELF (estático, não-PIE, entry `_start`, sem libc/crt); (5) UB adjacente a ABI — uso indevido de red zone, suposições sobre a direction flag.

**Files audited / Arquivos auditados:** `src/start.asm`, `src/syscall.asm`, `src/sys_exit.c`, `src/sys_write.c`, `src/sys_read.c`, `src/sys_mmap.c`, `include/syscall.h`. Confirmed (via `find`) that these two `.asm` files are the **entire** hand-written assembly surface of Camada 0 — no other file contains a `syscall`/`asm`/`__asm__` construct (grep against `src/*.c` returned only doc-comment mentions of the filename `syscall.asm`, not inline asm).

## Method / Método

**EN:** Read every instruction of `start.asm` and `syscall.asm` line-by-line against the ADR-0001/0003/0005 contracts. Then rebuilt from clean (`make clean && make build`, zero warnings) and cross-checked the **actual generated machine code**, not just the source, via `objdump -d -M intel` (Intel syntax, matching NASM) on the linked binaries in `build/bin/`, plus `readelf -h -l -d -S` for the ELF layout claims. Ran the full test suite (`make test`, 15/15 PASS) as a smoke baseline before judging. No product file was modified.

**PT:** Lida instrução a instrução de `start.asm` e `syscall.asm` contra os contratos do ADR-0001/0003/0005. Depois, build limpo (`make clean && make build`, zero warnings) e conferência cruzada do **código de máquina realmente gerado**, não só do source, via `objdump -d -M intel` (sintaxe Intel, casando com NASM) nos binários linkados em `build/bin/`, mais `readelf -h -l -d -S` para as alegações de layout ELF. Rodada a suíte completa (`make test`, 15/15 PASS) como baseline de fumaça antes do julgamento. Nenhum arquivo de produto foi alterado.

## Findings / Achados

### CRITICAL / CRÍTICO

None found. / Nenhum encontrado.

### IMPORTANT / IMPORTANTE

None found. / Nenhum encontrado.

### COSMETIC / COSMÉTICO

| # | File:line / Arquivo:linha | Finding / Achado |
|---|---|---|
| 1 | `src/syscall.asm:33` (`syscall3`) | **EN:** The header comment (lines 5-9) documents the C-ABI arg registers as `rdi,rsi,rdx,rcx,r8,r9` (correct, generic 6-arity note), but a reader skimming only the `syscall3:` line comment (`nr=rdi a1=rsi a2=rdx a3=rcx`) could momentarily wonder why `r10` isn't mentioned for a 3-arg syscall — it's correctly absent because `syscall3` only ever populates `rax/rdi/rsi/rdx` for the kernel, never touching `r10`. No code issue; a one-line clarifying comment (e.g. "`r10` unused: only 3 syscall args") would pre-empt the question for a future reader. Non-blocking. **PT:** O comentário de cabeçalho (linhas 5-9) documenta os registradores de arg da C-ABI como `rdi,rsi,rdx,rcx,r8,r9` (correto, nota genérica de 6-aridade), mas quem ler só o comentário da linha `syscall3:` (`nr=rdi a1=rsi a2=rdx a3=rcx`) pode se perguntar momentaneamente por que `r10` não aparece pra uma syscall de 3 args — está corretamente ausente porque `syscall3` só popula `rax/rdi/rsi/rdx` pro kernel, nunca toca `r10`. Não é problema de código; um comentário de uma linha (ex.: "`r10` não usado: só 3 args de syscall") evitaria a dúvida de um leitor futuro. Não bloqueia. |

## Verified conformant, with evidence / Verificado conforme, com evidência

**1. C-ABI → raw-syscall register reshuffle (`r10` replacing `rcx`) / Reorganização C-ABI → syscall crua (`r10` no lugar de `rcx`)**

`src/syscall.asm:27-50`, confirmed both at source and in the machine code emitted into `build/bin/exit42` (`objdump -d -M intel`):

```
syscall1: mov rax,rdi ; mov rdi,rsi ; syscall ; ret
syscall3: mov rax,rdi ; mov rdi,rsi ; mov rsi,rdx ; mov rdx,rcx ; syscall ; ret
syscall6: mov rax,rdi ; mov rdi,rsi ; mov rsi,rdx ; mov rdx,rcx ;
          mov r10,r8 ; mov r8,r9 ; mov r9,[rsp+8] ; syscall ; ret
```

`syscall1`/`syscall3` correctly never touch `r10` — they map 1/3 syscall args, so the 4th arg register is irrelevant and no shuffle is needed. `syscall6` is the only wrapper with a real 4th syscall arg, and it correctly moves it `r8→r10` (line `syscall.asm:46`) **before** clobbering `r8` at the following line (`syscall.asm:47`, `mov r8,r9`) — verified the write-before-overwrite ordering is safe: `r10` is written from `r8` first, only then is `r8` overwritten with the old `r9` (a5), only then is `r9` overwritten with the stack-spilled 7th C arg (a6, at `[rsp+8]`). No register is read after being clobbered. Disassembly of the linked `exit42` binary (`build/bin/exit42`, offset `0x401868`) reproduces this instruction-for-instruction: `mov r10,r8 / mov r8,r9 / mov r9,QWORD PTR [rsp+0x8] / syscall / ret`.

**2. Callee-saved registers (`rbx, rbp, r12-r15`) / Registradores callee-saved**

Neither `syscall1`, `syscall3`, `syscall6`, nor `_start` touch `rbx`, `rbp` (except `_start`'s intentional, ABI-conventional `xor rbp,rbp` zeroing at process entry — not a "restore" issue since there's no caller to break), `r12`, `r13`, `r14`, or `r15`. All three `syscallN` leaf functions only read/write `rax, rdi, rsi, rdx, rcx (read-only, as source of `r10`/`rdx` moves), r8, r9, r10` — the full caller-saved set, exactly as expected of thin ABI-glue leaves with no local state to preserve. Confirmed both by source read and by the `objdump` listing above (no `push rbx`/`mov r12,...` etc. anywhere in either `.asm` file).

**3. `syscall` instruction's own clobber of `rcx`/`r11` / Clobber de `rcx`/`r11` pela própria instrução `syscall`**

The `syscall` instruction destroys `rcx` (loaded with the return RIP) and `r11` (loaded with RFLAGS) as a side effect of the `SYSCALL`/`SYSRET` fast-transition mechanism — this is correct x86-64 kernel-entry hardware behavior, not a bug. Per the System V AMD64 ABI, `rcx` and `r11` are **caller-saved** (volatile) registers already, so the C compiler (clang) never assumes they survive *any* `call`, including a call to `syscall1`/`syscall3`/`syscall6`. Checked all four call-sites in the compiled object code (`sys_exit.o`, `sys_write.o`, `sys_read.o`, `sys_mmap.o` disassembly in `build/bin/test_alloc`): none of them read `rcx` or `r11` after the `call syscallN` returns — each wrapper's C body only ever consumes the `rax` return value. No ABI violation.

**4. 16-byte stack alignment before `call main` / Alinhamento de stack de 16B antes de `call main`**

`src/start.asm:39-53`, confirmed identical in the disassembly of `build/bin/exit42` at `_start` (offset `0x401820`):

```
_start:
    xor  rbp, rbp
    mov  rdi, [rsp]            ; argc
    lea  rsi, [rsp+8]          ; argv
    lea  rdx, [rsi+rdi*8+8]    ; envp = argv + (argc+1)*8
    and  rsp, -16              ; force 16-byte alignment
    call main
```

`argc`/`argv`/`envp` are captured into `rdi`/`rsi`/`rdx` **before** the `and rsp,-16` executes, so no live stack data is lost by the (potential) downward rounding. `and rsp,-16` guarantees `rsp % 16 == 0` immediately before `call main`, which — per the SysV convention that a `call` pushes an 8-byte return address — leaves `rsp % 16 == 8` on entry to `main`, exactly what a compiler-generated function prologue expects (the next `push` inside `main`, if any, restores 16-alignment). This is defensive: the Linux kernel already hands control to `_start` with `rsp` 16-aligned at process entry (well-established ELF/SysV process-startup convention, same one every `crt0`/`crt1` relies on) — the `and` is a correct belt-and-suspenders re-alignment, not a workaround for a real misalignment, matching the doc-comment's own stated rationale ("we do not trust the loader unconditionally").

The `argc`/`argv`/`envp` address arithmetic itself is correct: `rdi=[rsp]` (argc), `rsi=rsp+8` (`&argv[0]`), `rdx = rsi + argc*8 + 8` (`argv`'s own array occupies `argc` pointer slots plus one `NULL` terminator, so `envp[0]` starts exactly `(argc+1)*8` bytes past `argv[0]`) — verified against the documented Linux/SysV initial-stack layout in the file's own header comment and independently re-derived by hand; both agree.

**5. ELF layout (static, non-PIE, `_start` entry, no libc/crt) / Layout ELF**

`readelf -h build/bin/exit42`: `Tipo: EXEC` (not `DYN`/PIE), `Máquina: Advanced Micro Devices X86-64`, `Ponto de entrada: 0x401820` (matches `_start`'s address in the `objdump` symbol table). `readelf -l`: 4 program headers — 2× `LOAD` (`R E` text+rodata, `RW` bss) plus `GNU_STACK` with flags `RW` (**not** `RWE` — confirms the `.note.GNU-stack noalloc noexec nowrite` directive at the end of both `.asm` files correctly marks the stack non-executable at the toolchain level) — **no `PT_INTERP`, no `PT_DYNAMIC`**. `readelf -d`: "Não há secção dinâmica neste ficheiro" (no dynamic section) — confirms fully static, no dynamic loader dependency. `readelf -S`: only `.text/.rodata/.bss/.comment/.symtab/.strtab/.shstrtab` — no `.dynsym`/`.dynstr`/`.plt`/`.got.plt`, no libc/crt sections of any kind. All of this matches ADR-0005 exactly (`ld -nostdlib -static -no-pie -e _start`).

Additionally verified no stack-canary codegen leaked in despite being C23 (`-fno-stack-protector` honored): no `mov rax,fs:0x28` / `xor ... fs:0x28` canary pattern appears anywhere in the `sys_mmap`/`sys_write`/`sys_exit`/`_start` disassembly.

**6. ABI-adjacent UB: red zone, direction flag / UB adjacente a ABI: red zone, direction flag**

**Red zone (128B below `rsp`):** neither `.asm` file ever accesses memory at a negative offset from `rsp` without first adjusting `rsp` itself. `syscall6`'s only stack read is `[rsp+8]` — a **positive** offset (the caller-spilled 7th C argument, sitting *above* the return address, not in the red zone below it). `_start` performs no stack reads/writes below `rsp` at all. No red-zone misuse.

**Direction flag (DF):** neither file contains any string instruction (`movs*/stos*/cmps*/scas*`) whose behavior depends on DF, so no assumption about DF's state (set/clear) exists anywhere in the hand-written assembly. Not applicable to this audit's ASM surface; the pure-C libc-core files (`mem.c`, `str.c`) that implement `memcpy`/`memset`/etc. are compiled by clang from ordinary C loops (verified no inline `asm`/`__asm__` in any `src/*.c` — the only `grep` hits were doc-comments naming the file `syscall.asm`, not actual assembly), so DF correctness for those is the compiler's contract, outside this audit's ASM-focused scope.

**7. C↔ASM boundary argument-count/type correctness / Correção de contagem/tipo de argumento na fronteira C↔ASM**

Each `sys_*.c` wrapper's call to `syscallN` was checked against `syscallN`'s NASM-side arity: `sys_exit`→`syscall1` (1 arg after `nr`), `sys_write`/`sys_read`→`syscall3` (3 args), `sys_mmap`→`syscall6` (6 args, the `long off` 6th arg correctly spills to `[rsp+8]` in the compiler-generated call site — independently confirmed by disassembling `sys_mmap` in `build/bin/test_alloc`: `mov QWORD PTR [rsp],rax` immediately before `call syscall6`, which lands at `[rsp+8]` from `syscall6`'s post-`call` perspective, exactly where the NASM code reads it). All pointer/size arguments are explicitly cast to `long` before crossing into the raw-syscall wrappers (`(long)buf`, `(long)count`, `(long)addr`, etc.), matching the "raw syscall ABI is all-`long`" contract documented in each wrapper's header comment. `include/syscall.h`'s prototypes (`long syscall1(long,long)`, `long syscall3(long,long,long,long)`, `long syscall6(long,long,long,long,long,long,long)`) match the NASM `global` symbol arities exactly — no signature drift between the C declaration and the ASM definition.

## Conclusion / Conclusão

**EN:** The Camada 0 hand-written assembly surface (`start.asm`, `syscall.asm`) and its C boundary (`sys_exit.c`, `sys_write.c`, `sys_read.c`, `sys_mmap.c`) are **fully conformant** with the System V AMD64 ABI as ratified in ADR-0001/0003/0005, verified both by source inspection and by disassembling/inspecting the actual toolchain output (`clang`+`nasm`+`ld`). Zero CRITICAL, zero IMPORTANT findings; one non-blocking COSMETIC comment-clarity suggestion. `AUD-ABI`'s "Estado Auditado" column may move to `✓` (aprovado) once the líder ratifies this report; it does not block `REL-TAG` on its own (still needs `AUD-SEC` + `F1` per the TODO.md dependency graph).

**PT:** A superfície de assembly escrito à mão da Camada 0 (`start.asm`, `syscall.asm`) e sua fronteira C (`sys_exit.c`, `sys_write.c`, `sys_read.c`, `sys_mmap.c`) estão **totalmente conformes** à ABI System V AMD64 conforme ratificada no ADR-0001/0003/0005, verificado tanto por inspeção de source quanto por desmontagem/inspeção da saída real do toolchain (`clang`+`nasm`+`ld`). Zero achados CRÍTICOS, zero IMPORTANTES; uma sugestão COSMÉTICA de clareza de comentário, não-bloqueante. A coluna "Estado Auditado" de `AUD-ABI` pode passar a `✓` (aprovado) assim que o líder ratificar este relatório; ela sozinha não libera o `REL-TAG` (ainda depende de `AUD-SEC` + `F1` no grafo de pré-requisitos do TODO.md).

---

## Delta re-audit 2026-07-19 / Re-auditoria delta 2026-07-19

- **Auditor:** embedded-firmware-engineer (same auditor as the 2026-07-09 report — method continuity, per `docs/auditoria/AUD-C0-PLAN.md` §3 / mesmo auditor do relatório de 2026-07-09 — continuidade de método, conforme `docs/auditoria/AUD-C0-PLAN.md` §3).
- **Scope item / Item da TODO.md:** `AUD-ABI-Δ` + `AUD-C0-GATE` (Onda 4, `docs/auditoria/AUD-C0-PLAN.md` §2.1/§2.3). **Delta only** — everything the 2026-07-09 report already verified (`syscall1/3/6`, `_start`, `sys_exit/write/read/mmap`) is **unchanged** at this HEAD and is **not** re-derived below, only re-confirmed where the delta touches shared code paths. / **Só o delta** — tudo que o relatório de 2026-07-09 já verificou (`syscall1/3/6`, `_start`, `sys_exit/write/read/mmap`) está **inalterado** neste HEAD e **não** é re-derivado abaixo, só re-confirmado onde o delta toca caminhos de código compartilhados.
- **Verdict / Veredito:** **CONFORME-COM-RESSALVAS — one 🟠 IMPORTANT finding (new, undocumented), zero 🔴 CRITICAL.** / **CONFORME-COM-RESSALVAS — um achado 🟠 IMPORTANTE (novo, não-documentado), zero 🔴 CRÍTICO.**

### Scope of the delta / Escopo do delta

**EN:** Per `AUD-C0-PLAN.md` §2.1: (1) `syscall2` (`src/syscall.asm`), re-added by SOV-ALLOC (W15) for `sys_munmap`; (2) `src/sys_munmap.c`, the new C↔ASM boundary calling it; (3) `src/core_api.c` + the mixed `libcore.a` ↔ hosted-C++ boundary of ADR-0009 (stack alignment, red zone, `glx_` prefix leak-tightness, `nm -u` gate).

**PT:** Conforme `AUD-C0-PLAN.md` §2.1: (1) `syscall2` (`src/syscall.asm`), readicionada pelo SOV-ALLOC (W15) pro `sys_munmap`; (2) `src/sys_munmap.c`, a fronteira C↔ASM nova que a chama; (3) `src/core_api.c` + a fronteira mista `libcore.a` ↔ C++ hosted da ADR-0009 (alinhamento de stack, red zone, estanqueidade do prefixo `glx_`, gate `nm -u`).

### Method / Método

**EN:** Rebuilt from clean with `TMPDIR=/var/tmp` (`make clean && make build && make libcore && make libcore-test`, zero warnings across all four). Cross-checked the actual generated machine code (not just source) via `objdump -d -M intel` and `readelf` on the rebuilt binaries in `build/bin/` and the archive members in `build/libcore.a`. Ran `strace -f -c` on **every** binary in `build/bin/` (16 freestanding gate programs + `libcore_consumer`) to inventory real syscalls, following forks (`-f`) since `libcore_consumer` forks 3 children to prove the ADR-0009 heap-ownership boundary. Ran every gate listed in `AUD-C0-PLAN.md` §2.3 that exists at this HEAD (`sanitize-hint` does not exist yet — it is `security-engineer`'s deliverable under `AUD-SEC-Δ`, out of my write scope; noted as pending below, not scored as a failure of this chapter).

**PT:** Rebuild limpo com `TMPDIR=/var/tmp` (`make clean && make build && make libcore && make libcore-test`, zero warnings nos quatro). Conferência cruzada do código de máquina realmente gerado (não só do source) via `objdump -d -M intel` e `readelf` nos binários rebuiltados em `build/bin/` e nos membros do archive em `build/libcore.a`. Rodado `strace -f -c` em **todo** binário de `build/bin/` (16 programas-gate freestanding + `libcore_consumer`) pra inventariar as syscalls reais, seguindo forks (`-f`) já que `libcore_consumer` forka 3 filhos pra provar a fronteira de ownership de heap da ADR-0009. Rodado todo gate listado em `AUD-C0-PLAN.md` §2.3 que existe neste HEAD (`sanitize-hint` ainda não existe — é entregável do `security-engineer` sob `AUD-SEC-Δ`, fora do meu escopo de escrita; anotado como pendente abaixo, não contado como falha deste capítulo).

### Findings / Achados

#### CRITICAL / CRÍTICO

None found. / Nenhum encontrado.

#### IMPORTANT / IMPORTANTE

| # | File:line / Arquivo:linha | Finding / Achado |
|---|---|---|
| 1 | `Makefile:39` (`CXXFLAGS`) + the `libcore.a`↔hosted-C++ link at `Makefile:570-571` | **EN:** Linking `build/libcore.a` (compiled `-fno-pic`, `Makefile:23`, mandatory for Layer 0's freestanding contract) into an ordinary hosted C++ program **silently forces the whole consumer binary out of PIE**. Verified live: `readelf -h build/bin/libcore_consumer` reports `Tipo: EXEC` (not `DYN`), entry point `0x400410` — a fixed, non-randomized load address — even though `CXXFLAGS` (`Makefile:39`) requests no `-no-pie` and modern `clang++`/`ld` default to PIE. `ld` silently falls back to a non-PIE `ET_EXEC` because it cannot place non-position-independent object code (the archive members, all built with `-fno-pic`) into a position-independent image; it does **not** error, so nothing in the current build flags this loss of a real OS-level hardening (ASLR) to whoever links `libcore.a`. This is a **new, undocumented cost of ADR-0009's Option A**, distinct from the "no longer zero-libc-observable" cost the ADR already accepts and states explicitly — this one is nowhere named. It matters concretely because the ADR's own stated destination for `libcore.a` is exactly a hosted C++ shell that will eventually parse **hostile input** (SFNT — already flagged as such in `AUD-C0-PLAN.md` §1/§2.2, and `src/sfnt.c`/`src/raster.c`/`src/hint.c` are already archive members today, see `nm -u build/libcore.a` evidence below) — losing ASLR system-wide on that binary removes a real line of defense-in-depth exactly where a hostile-input parser lives. Not exploitable by itself (no memory corruption, no ABI violation — every function call across the boundary is empirically correct, see "Verified conformant" below), hence 🟠 not 🔴. **Remediation is documentation, not necessarily code**: either (a) add this as an explicit accepted-cost addendum to ADR-0009 (parallel to the existing zero-libc-purity one) so every future consumer of `libcore.a` makes an informed choice, or (b) evaluate a `-fpic`-compiled variant of the archive for hosted PIE consumers (architecture decision, not mine to make — `software-architect`/líder). Flagging, not fixing, per this chapter's write scope. **PT:** Linkar a `build/libcore.a` (compilada `-fno-pic`, `Makefile:23`, obrigatório pelo contrato freestanding da Camada 0) num programa C++ hosted comum **força em silêncio o binário do consumidor inteiro pra fora de PIE**. Verificado ao vivo: `readelf -h build/bin/libcore_consumer` reporta `Tipo: EXEC` (não `DYN`), ponto de entrada `0x400410` — um endereço de carga fixo, não-aleatorizado — mesmo o `CXXFLAGS` (`Makefile:39`) não pedindo `-no-pie` nenhum e `clang++`/`ld` modernos default pra PIE. O `ld` cai em silêncio pra um `ET_EXEC` não-PIE porque não consegue colocar código-objeto não-posição-independente (os membros do archive, todos compilados `-fno-pic`) dentro de uma imagem posição-independente; ele **não** dá erro, então nada no build atual sinaliza essa perda de um hardening real de nível de SO (ASLR) pra quem linkar a `libcore.a`. Este é um **custo novo, não-documentado, da Opção A da ADR-0009**, distinto do custo "deixa de ser zero-libc-observável" que a ADR já aceita e enuncia explicitamente — este aqui não é nomeado em lugar nenhum. Importa concretamente porque o próprio destino declarado da ADR pra `libcore.a` é exatamente uma casca C++ hosted que vai eventualmente parsear **input hostil** (SFNT — já sinalizado como tal em `AUD-C0-PLAN.md` §1/§2.2, e `src/sfnt.c`/`src/raster.c`/`src/hint.c` já são membros do archive hoje, ver evidência `nm -u build/libcore.a` abaixo) — perder ASLR no binário inteiro remove uma linha real de defesa-em-profundidade exatamente onde mora um parser de input hostil. Não é explorável por si só (sem corrupção de memória, sem violação de ABI — toda chamada através da fronteira está empiricamente correta, ver "Verificado conforme" abaixo), daí 🟠 e não 🔴. **Remediação é documentação, não necessariamente código**: ou (a) acrescentar isto como um adendo explícito de custo-aceito na ADR-0009 (paralelo ao de pureza-zero-libc já existente) pra todo futuro consumidor da `libcore.a` fazer uma escolha informada, ou (b) avaliar uma variante `-fpic` do archive pra consumidores PIE hosted (decisão de arquitetura, não minha pra tomar — `software-architect`/líder). Sinalizando, não corrigindo, conforme o escopo de escrita deste capítulo. |

#### COSMETIC / COSMÉTICO

| # | File:line / Arquivo:linha | Finding / Achado |
|---|---|---|
| 2 | `src/syscall.asm:36` (`syscall2`) | **EN:** Same observation as COSMETIC finding #1 of the 2026-07-09 report, now extended to the new `syscall2`: its line comment (`nr=rdi a1=rsi a2=rdx`) correctly never mentions `r10`/`rcx`/`rdx→rcx` reshuffling because a 2-arg syscall needs neither — but a reader skimming only that line could momentarily wonder why, same as with `syscall1`/`syscall3` before it. No code issue, non-blocking; a one-line clarifying comment would pre-empt the question, same remediation already on file for finding #1. **PT:** Mesma observação do achado COSMÉTICO #1 do relatório de 2026-07-09, agora estendida à nova `syscall2`: seu comentário de linha (`nr=rdi a1=rsi a2=rdx`) corretamente nunca menciona reorganização de `r10`/`rcx`/`rdx→rcx` porque uma syscall de 2 args não precisa de nenhuma das duas — mas quem ler só essa linha pode se perguntar momentaneamente por quê, igual já acontecia com `syscall1`/`syscall3` antes dela. Não é problema de código, não-bloqueante; um comentário de uma linha evitaria a dúvida, mesma remediação já registrada pro achado #1. |

### Verified conformant, with evidence / Verificado conforme, com evidência

**1. `syscall2` register reshuffle and clobber ordering / Reorganização de registrador e ordem de clobber da `syscall2`**

Source (`src/syscall.asm:36-41`) and the actual machine code linked into `build/bin/test_alloc` (the one gate program that calls `sys_munmap`, via `objdump -d -M intel`) match instruction-for-instruction:

```
syscall2:
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    syscall
    ret
```

Two raw syscall args (`addr`, `len`), so only `rax/rdi/rsi` are touched — `r10`/`rdx`/`rcx` are correctly never referenced, consistent with the reshuffle-only-what-is-needed pattern already verified for `syscall1`/`syscall3` in the 2026-07-09 report. No clobber-before-read hazard is possible with only 2 registers in play (unlike `syscall6`'s documented `r8→r10`-before-`r8`-is-overwritten ordering, which remains unchanged and still correct at this HEAD — re-confirmed via `objdump` on `build/bin/exit42`/`test_alloc`, identical to the original report's listing).

**2. `sys_munmap.c` C↔ASM boundary / Fronteira C↔ASM do `sys_munmap.c`**

`src/sys_munmap.c:31-33` casts both `void* addr` and `size_t len` to `long` before crossing into `syscall2`, matching the "raw syscall ABI is all-`long`" contract every other `sys_*.c` wrapper already follows (verified against `include/syscall.h`'s `long syscall2(long, long, long)` prototype — arity matches the NASM `global syscall2` definition exactly, no signature drift).

**3. `libcore.a` boundary — stack alignment, red zone, `glx_` prefix tightness / Fronteira `libcore.a` — alinhamento de stack, red zone, estanqueidade do prefixo `glx_`**

- **Stack alignment (16B at `call`):** not a new risk. Every function crossing the `libcore.a` boundary (`glx_malloc`/`glx_free`/`glx_realloc`/`glx_memcpy`/`glx_memset` and everything they call internally) is an **ordinary** clang-compiled C function, not a hand-rolled entry point like `_start` — both the freestanding side and the hosted C++ caller side independently honor the System V AMD64 C-ABI's own 16B-at-`call` contract regardless of `-ffreestanding`/`-nostdlib`, which govern *runtime environment assumptions*, not the calling convention itself. Empirically exercised hard by `libcore_consumer`'s 16/16 PASS checks (`make libcore-test` output below), including grow/shrink `realloc` cycles that would corrupt on any real misalignment — none did.
- **Red zone:** no new exposure. The functions crossing the boundary are ordinary leaf-adjacent C functions (not signal handlers, not code that can be asynchronously interrupted below `rsp`), so the SysV red-zone hazard class does not apply here regardless of which side of the boundary calls which — same conclusion as the 2026-07-09 report's finding 6, unchanged.
- **`glx_` prefix tightness (`nm -u build/libcore.a`, pasted below):** every archive member (`alloc.o`, `conv.o`, `core_api.o`, `hint.o`, `mem.o`, `printf.o`, `raster.o`, `sfnt.o`, `str.o`, `sys_exit.o`, `sys_mmap.o`, `sys_munmap.o`, `sys_read.o`, `sys_write.o`, plus the reused `start.o`/`syscall.o`) references **only** `glx_core_*_impl` names, this project's own `sys_*`/`syscall1..6` names, or `main` (an intentional undefined reference in `start.o`, resolved by whatever program links it, freestanding or otherwise) — **zero bare libc-shaped identifier** (`malloc`, `strlen`, `memcpy`, …) anywhere in the archive, confirming the `$(CORE_RENAME_FLAGS)` mechanism (`Makefile:521-532`) and its prior CRITICAL fix (post-`decb2fb`, per `src/core_api.c`'s own header) still hold at this HEAD. `tools/check_libcore_symbols.sh` (positive whitelist, run by `make libcore-test`) independently confirms the same thing from the exported-symbol side.

### AUD-C0-GATE evidence / Evidência do AUD-C0-GATE

**`nm -u` — zero-libc invariant across every binary / invariante zero-libc em todo binário:**

```
$ for f in build/bin/*; do echo "--- $f ---"; nm -u "$f"; done
--- build/bin/echo_stdin ---        (empty)
--- build/bin/exit42 ---            (empty)
--- build/bin/hello ---             (empty)
--- build/bin/libcore_consumer ---  U _exit@GLIBC_2.2.5, U exit@GLIBC_2.2.5, U fork@GLIBC_2.2.5,
                                    U free@GLIBC_2.2.5, w __gmon_start__, U __libc_start_main@GLIBC_2.34,
                                    U malloc@GLIBC_2.2.5, U memcmp@GLIBC_2.2.5, U memset@GLIBC_2.2.5,
                                    U perror@GLIBC_2.2.5, U printf@GLIBC_2.2.5, U waitpid@GLIBC_2.2.5
--- build/bin/negative_probe ---, printf_e2e, selftest, test_alloc, test_alloc_free, test_conv,
    test_hint, test_mem, test_printf, test_raster, test_sfnt, test_str ---  (all empty)
```

**EN:** All 16 freestanding gate binaries show `nm -u` **empty** — zero undefined symbols, i.e. zero libc dependency, at the strictest possible level (no libc symbol reference exists to even *attempt* resolving). `libcore_consumer` is the sole, **by-design**, exception (ADR-0009 explicitly never claims zero-libc purity for the hosted-shell side) — its undefined-symbol list is exactly the ordinary glibc/libstdc++ surface a `fork`+`malloc`+`printf` C++ program needs, nothing more, nothing unexpected.

**PT:** Os 16 binários-gate freestanding mostram `nm -u` **vazio** — zero símbolo não-resolvido, ou seja, zero dependência de libc, no nível mais estrito possível (não existe referência a símbolo de libc nem pra *tentar* resolver). O `libcore_consumer` é a única exceção, **por design** (a ADR-0009 nunca reivindica pureza zero-libc pro lado casca-hosted) — sua lista de símbolos não-resolvidos é exatamente a superfície glibc/libstdc++ comum que um programa C++ `fork`+`malloc`+`printf` precisa, nada mais, nada inesperado.

**`strace -f -c` — syscall inventory / inventário de syscalls:**

| Binary / Binário | Syscalls observed (beyond `execve`) / Syscalls observadas (além de `execve`) |
|---|---|
| `echo_stdin` | `read` |
| `exit42` | (none) |
| `hello` | `write` |
| `negative_probe` | `write` |
| `printf_e2e` | `write` (×2) |
| `selftest` | (none) |
| `test_alloc` | `write`, `mmap` (×4) |
| `test_alloc_free` | `write`, `mmap` (×3), `munmap` (×2) |
| `test_conv`, `test_mem`, `test_printf`, `test_raster`, `test_sfnt`, `test_str` | `write` |
| `test_hint` | `write` |
| `libcore_consumer` (hosted, `-f` follows 3 forked children) | `wait4`(×3), `clone`(×3), `mmap`(×24), `mprotect`(×6), `openat`(×5), `fstat`(×6), `munmap`(×3), `close`(×5), `read`(×4), `rt_sigprocmask`(×9), `brk`(×3), `pread64`(×2), `write`, `access`, `rseq`, `arch_prctl`, `set_tid_address`, `set_robust_list`(×4), `futex`, `getrandom`, `prlimit64`, `writev`, `getpid`, `gettid`, `tgkill` |

**EN:** Every freestanding binary emits **only** syscalls this project itself implements (`write`, `read`, `mmap`, `munmap` — exactly `sys_write`/`sys_read`/`sys_mmap`/`sys_munmap`), confirming at the syscall-inventory level (not just symbol-table level) that no hidden libc is doing I/O or allocation on the process's behalf. `libcore_consumer`'s broader inventory is the expected footprint of an ordinary dynamically-linked, forking hosted C++ program (dynamic loader `openat`/`mmap`/`mprotect`/`fstat`, `clone`/`wait4` for its 3 deliberate boundary-misuse forks, `getrandom`/`arch_prctl`/`rseq` from glibc/CRT startup) — nothing unexpected, nothing that suggests a hidden network/file/process surface.

**PT:** Todo binário freestanding emite **só** as syscalls que este projeto mesmo implementa (`write`, `read`, `mmap`, `munmap` — exatamente `sys_write`/`sys_read`/`sys_mmap`/`sys_munmap`), confirmando no nível de inventário-de-syscall (não só de tabela-de-símbolo) que nenhuma libc escondida está fazendo I/O ou alocação em nome do processo. O inventário mais amplo do `libcore_consumer` é a pegada esperada de um programa C++ hosted comum, dinamicamente linkado e que forka (carregador dinâmico `openat`/`mmap`/`mprotect`/`fstat`, `clone`/`wait4` pelos seus 3 forks deliberados de mau-uso de fronteira, `getrandom`/`arch_prctl`/`rseq` do startup glibc/CRT) — nada inesperado, nada que sugira superfície escondida de rede/arquivo/processo.

**Gate scoreboard / placar de gates (`AUD-C0-PLAN.md` §2.3):**

| Gate | Result / Resultado |
|---|---|
| `make clean && make build` | 🟢 verde — zero warnings |
| `make libcore` | 🟢 verde — zero warnings |
| `make libcore-test` (`libcore_consumer` 16/16 checks + `check_libcore_symbols.sh`) | 🟢 verde |
| `make test` (16/16 gate programs) | 🟢 verde |
| `make test-negative` | 🟢 verde |
| `make check-static` (cppcheck + clang --analyze + SPDX + syscall_nums sync) | 🟢 verde — 0 achados |
| `make test-mem` (valgrind × test_alloc/test_mem) | 🟢 verde — 0 erros (limitação de malloc-hook documentada, inalterada) |
| `make sanitize-sfnt` | 🟢 verde — ASan/UBSan silenciosos |
| `make sanitize-raster` | 🟢 verde — ASan/UBSan silenciosos |
| `make sanitize-hint` | ⚪ não existe neste HEAD ainda — entregável do `security-engineer` sob `AUD-SEC-Δ` (`AUD-C0-PLAN.md` §2.2), fora do escopo de escrita deste capítulo |

### Conclusion / Conclusão

**EN:** The Δ surface (`syscall2`, `sys_munmap.c`, the `libcore.a`↔hosted-C++ boundary) is ABI-conformant with System V AMD64 as ratified in ADR-0001/0003/0005/0009 — the reshuffle/clobber/argument-count discipline the 2026-07-09 report already verified for `syscall1/3/6` holds identically for `syscall2`, and every crossing of the mixed `libcore.a` boundary is empirically correct (16/16 `libcore_consumer` checks, zero `nm -u` leakage, zero unexpected syscall). One new 🟠 IMPORTANT finding — the hosted `libcore.a` consumer silently loses PIE/ASLR, an undocumented cost of ADR-0009's Option A distinct from the zero-libc-purity cost the ADR already names — is raised for the líder/`software-architect` to disposition (documentation addendum vs. a `-fpic` archive variant); it does not corrupt memory or violate the ABI itself. All `AUD-C0-GATE` checks that exist at this HEAD are green; `sanitize-hint` is pending the parallel `AUD-SEC-Δ` agent. Given the finding is real, new, and not yet dispositioned, this report's overall delta verdict is **CONFORME-COM-RESSALVAS**, not a clean CONFORME — per the wave's own gate rule (`AUD-C0-PLAN.md` §4), the 🟠 should be resolved (fixed or formally accepted-and-documented) within the wave before `REL-TAG core-v0.4.0`.

**PT:** A superfície Δ (`syscall2`, `sys_munmap.c`, a fronteira `libcore.a`↔C++ hosted) é conforme à ABI System V AMD64 conforme ratificada no ADR-0001/0003/0005/0009 — a disciplina de reorganização/clobber/contagem-de-argumento que o relatório de 2026-07-09 já verificou pra `syscall1/3/6` vale identicamente pra `syscall2`, e toda travessia da fronteira mista `libcore.a` está empiricamente correta (16/16 checks do `libcore_consumer`, zero vazamento em `nm -u`, zero syscall inesperada). Um achado novo 🟠 IMPORTANTE — o consumidor hosted da `libcore.a` perde PIE/ASLR em silêncio, um custo não-documentado da Opção A da ADR-0009 distinto do custo de pureza-zero-libc que a ADR já nomeia — é levantado pro líder/`software-architect` dispor (adendo de documentação vs. uma variante `-fpic` do archive); ele não corrompe memória nem viola a ABI em si. Todo gate do `AUD-C0-GATE` que existe neste HEAD está verde; `sanitize-hint` está pendente do agente paralelo do `AUD-SEC-Δ`. Dado que o achado é real, novo, e ainda não disposto, o veredito geral deste delta é **CONFORME-COM-RESSALVAS**, não um CONFORME limpo — pela própria regra de gate da onda (`AUD-C0-PLAN.md` §4), o 🟠 deveria ser resolvido (corrigido ou formalmente aceito-e-documentado) dentro da onda antes do `REL-TAG core-v0.4.0`.
