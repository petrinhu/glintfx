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
