// SPDX-License-Identifier: MPL-2.0
// EN: D1 -- see include/mem.h for the contract. Byte-at-a-time implementations, deliberately
//     simple (no word-at-a-time/SIMD tricks -- correctness and auditability first; throughput
//     optimization is out of scope for this increment and would be premature without a
//     benchmark, YAGNI). Every loop walks `unsigned char*` exclusively: `unsigned char` is the
//     one C type the standard guarantees can alias any object's raw bytes without violating
//     strict aliasing, and it is what memcmp's byte-ordering contract is defined in terms of.
//
//     THE `-fno-builtin` GOTCHA (why this file looks the way it does): clang's LOOP-IDIOM
//     RECOGNITION pass can pattern-match a hand-written `for (i=0;i<n;i++) dst[i]=src[i];`
//     loop and rewrite it into a call to `memcpy` -- even INSIDE this very `memcpy`'s own
//     body, which would be a self-recursive call to a symbol this freestanding build has no
//     runtime support for (infinite recursion / stack smash, not a compile error). Same risk
//     applies to memset (-> `memset`) and byte-comparison loops (-> `memcmp`/`bcmp`). The
//     Makefile's `-fno-builtin` flag (CFLAGS) is a BLANKET opt-out: it tells clang "you may
//     not assume any function named like a libc function behaves like the libc one, and you
//     may not silently synthesize calls to libc functions this program never wrote" --
//     covering the whole `-fno-builtin-memcpy`/`-fno-builtin-memset`/`-fno-builtin-memmove`/
//     -fno-builtin-memcmp` family at once. VERIFIED empirically for this exact file after
//     building (see task report): `nm build/obj/mem.o` shows ONLY `T memcpy`/`T memset`/
//     `T memmove`/`T memcmp` (defined, this file) -- no `U memcpy` etc. (undefined = would-be
//     libc call) -- and `objdump -d -M intel` confirms none of the four function bodies
//     contains a `call` instruction at all, let alone a self-call. No source-level workaround
//     (`__attribute__((optnone))`, deliberately obfuscated loop shape, etc.) was needed; the
//     Makefile's existing flag was already sufficient.
// PT: D1 -- ver include/mem.h pro contrato. Implementacoes byte-a-byte, deliberadamente
//     simples (sem truques de word-a-word/SIMD -- corretude e auditabilidade primeiro;
//     otimizacao de throughput esta fora do escopo deste incremento e seria prematura sem um
//     benchmark, YAGNI). Todo laco percorre exclusivamente `unsigned char*`: `unsigned char` e'
//     o unico tipo C que o padrao garante poder aliasar os bytes crus de qualquer objeto sem
//     violar strict aliasing, e e' em termos dele que o contrato de ordenacao de bytes do
//     memcmp e' definido.
//
//     O GOTCHA `-fno-builtin` (por que este arquivo parece do jeito que parece): o passo de
//     LOOP-IDIOM RECOGNITION do clang pode reconhecer o padrao de um laco escrito a mao
//     `for (i=0;i<n;i++) dst[i]=src[i];` e reescreve-lo numa chamada a `memcpy` -- inclusive
//     DENTRO do corpo deste proprio `memcpy`, o que seria uma auto-chamada recursiva a um
//     simbolo que este build freestanding nao tem suporte de runtime nenhum (recursao
//     infinita/estouro de pilha, nao um erro de compilacao). Mesmo risco vale pro memset
//     (-> `memset`) e lacos de comparacao de byte (-> `memcmp`/`bcmp`). A flag `-fno-builtin`
//     do Makefile (CFLAGS) e' um opt-out GERAL: diz ao clang "voce nao pode assumir que
//     nenhuma funcao com nome de funcao de libc se comporta como a da libc, e nao pode
//     sintetizar silenciosamente chamadas a funcoes de libc que este programa nunca escreveu"
//     -- cobrindo de uma vez toda a familia `-fno-builtin-memcpy`/`-fno-builtin-memset`/
//     `-fno-builtin-memmove`/`-fno-builtin-memcmp`. VERIFICADO empiricamente pra este arquivo
//     exato apos buildar (ver relatorio da tarefa): `nm build/obj/mem.o` mostra APENAS
//     `T memcpy`/`T memset`/`T memmove`/`T memcmp` (definidos, este arquivo) -- nenhum
//     `U memcpy` etc. (undefined = seria chamada a libc) -- e `objdump -d -M intel` confirma
//     que nenhum dos quatro corpos de funcao contem instrucao `call` alguma, muito menos uma
//     auto-chamada. Nenhum contorno em nivel de fonte (`__attribute__((optnone))`, forma de
//     laco deliberadamente ofuscada, etc.) foi necessario; a flag ja existente do Makefile ja
//     foi suficiente.
// Copyright (c) 2026 Petrus Silva Costa
#include "mem.h"

void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

void* memset(void* dst, int c, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    unsigned char byte = (unsigned char)c;
    for (size_t i = 0; i < n; i++) {
        d[i] = byte;
    }
    return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    // EN: AUD-C0-1 (AUDIT_FIND.md): comparing `d`/`s` directly with `<`/`>` would be UNDEFINED
    //     BEHAVIOUR -- relational comparison between pointers is only defined by the C standard
    //     when both operands point into (or one-past) the SAME array/object (C23 6.5.9p6).
    //     `dst`/`src` here are two independent objects handed in by the caller; the standard
    //     gives no guarantee about their relative address order, even though on this project's
    //     one and only target (x86-64 Linux, flat address space, no segmentation) the raw
    //     pointer comparison happens to produce the ordering we want in practice. Casting both
    //     to `uintptr_t` (an unsigned INTEGER type, include/types.h) sidesteps the
    //     object-pointer rule entirely: ordinary unsigned-integer comparison is always
    //     well-defined, and the numeric value of a pointer cast to `uintptr_t` is guaranteed (on
    //     this project's only target) to reflect its address. This changes only the comparison
    //     OPERANDS, not the algorithm -- the overlap-safety reasoning in the two branches below
    //     is unchanged.
    // PT: AUD-C0-1 (AUDIT_FIND.md): comparar `d`/`s` diretamente com `<`/`>` seria COMPORTAMENTO
    //     INDEFINIDO -- comparacao relacional entre ponteiros so' e' definida pelo padrao C
    //     quando os dois operandos apontam pro MESMO array/objeto (ou um-alem-do-fim dele) (C23
    //     6.5.9p6). `dst`/`src` aqui sao dois objetos independentes recebidos de quem chamou; o
    //     padrao nao da' garantia nenhuma sobre a ordem relativa de endereco deles, mesmo que no
    //     unico alvo deste projeto (x86-64 Linux, espaco de enderecos plano, sem segmentacao) a
    //     comparacao crua de ponteiro acabe produzindo na pratica a ordenacao que queremos.
    //     Converter os dois pra `uintptr_t` (um tipo INTEIRO sem sinal, include/types.h) contorna
    //     inteiramente a regra de ponteiro-de-objeto: comparacao comum de inteiro sem sinal e'
    //     sempre bem-definida, e o valor numerico de um ponteiro convertido pra `uintptr_t` e'
    //     garantido (no unico alvo deste projeto) refletir seu endereco. Isso muda so' os
    //     OPERANDOS da comparacao, nao o algoritmo -- o raciocinio de seguranca-de-overlap nos
    //     dois ramos abaixo continua o mesmo.
    uintptr_t du = (uintptr_t)d;
    uintptr_t su = (uintptr_t)s;

    // EN: No overlap risk (or dst < src): plain forward copy is safe -- by the time we would
    //     ever write into a byte that is still needed as source, we have not reached it yet
    //     (dst[i] never catches up to s+i while writing forward when du < su, and when
    //     du == su it is a no-op copy either way).
    // PT: Sem risco de overlap (ou dst < src): copia forward simples e' segura -- no momento
    //     em que chegariamos a escrever num byte ainda necessario como origem, ainda nao o
    //     alcancamos (dst[i] nunca alcanca s+i escrevendo pra frente quando du < su, e quando
    //     du == su e' uma copia no-op de qualquer forma).
    if (du < su) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (du > su) {
        // EN: dst > src: forward copy would overwrite source bytes before they are read
        //     (whenever the regions overlap). Copy BACKWARD instead, from the last byte to
        //     the first, so every source byte is read before the write that could clobber it.
        // PT: dst > src: copia forward sobrescreveria bytes da origem antes de serem lidos
        //     (sempre que as regioes se sobrepoem). Em vez disso copia PRA TRAS, do ultimo
        //     byte ao primeiro, entao todo byte de origem e' lido antes da escrita que
        //     poderia corrompe-lo.
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    // EN: du == su: same address, nothing to do (also correctly handled by falling through
    //     with neither branch taken).
    // PT: du == su: mesmo endereco, nada a fazer (tambem tratado corretamente caindo adiante
    //     sem tomar nenhum dos dois ramos).

    return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            // EN: Both operands are already `unsigned char` -- the subtraction below is an
            //     unsigned-to-int widening, never a signed-`char` comparison, which is what
            //     makes the 0xFF-vs-0x01 (255 > 1) case come out positive as required.
            // PT: Ambos operandos ja sao `unsigned char` -- a subtracao abaixo e' um
            //     alargamento unsigned-pra-int, nunca uma comparacao de `char` signed, o que
            //     e' o que faz o caso 0xFF-vs-0x01 (255 > 1) sair positivo como exigido.
            return (int)pa[i] - (int)pb[i];
        }
    }
    return 0;
}
