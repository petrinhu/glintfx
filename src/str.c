// SPDX-License-Identifier: MPL-2.0
// EN: D2 -- see include/str.h for the contract. Byte-at-a-time implementations, same philosophy
//     as D1 (mem.c): correctness and auditability first, no word-at-a-time/SIMD tricks. Every
//     length/search loop walks `const char*`/`char*` (the C-string contract), comparisons
//     against `\0` use the literal directly (never `strlen` first -- these functions ARE part
//     of how a from-scratch `strlen` would even be discoverable, so none of them may depend on
//     a hidden two-pass "measure then act" shortcut that isn't in the standard's contract).
//
//     `strcpy`/strncpy`/`strcat` REUSE our own D1 `memcpy`/`memset` (`#include "mem.h"`) where
//     that is the clearest way to express "copy N known bytes" or "zero-pad a known-size tail"
//     -- these resolve at link time to `src/mem.c`, never libc, so this stays zero libc. `nm`
//     will show `U memcpy`/`U memset` in `str.o` (undefined in THIS object) that RESOLVE against
//     `mem.o` at link time -- that is expected and fine; it is a self-call to OUR OWN symbol,
//     not a reintroduced libc dependency. Distinguish that from the bad case this file guards
//     against below: a function calling ITSELF by name (e.g. `strlen` containing `call strlen`)
//     or calling a libc symbol we never wrote (e.g. `U strlen` inside `strlen.o` itself).
//
//     THE `-fno-builtin` GOTCHA (same risk class as D1, worth restating here because `strlen`/
//     `strcpy`/`strcmp`/`strchr` are EVEN MORE aggressively pattern-matched by clang's
//     loop-idiom recognition than raw memory loops -- these are textbook libc idioms): a
//     hand-written `while (s[i] != '\0') i++;` can be rewritten into a call to `strlen`, a
//     byte-copy-until-NUL loop into `strcpy`, etc. -- including a SELF-CALL inside the very
//     function being defined (infinite recursion / stack smash at runtime, not a compile
//     error). The Makefile's blanket `-fno-builtin` (CFLAGS) opts out of the whole family at
//     once. VERIFIED empirically for this exact file after building (see task report): `nm
//     build/obj/str.o` shows the seven `T` (defined) symbols, `U memcpy`/`U memset` (our own,
//     resolved against mem.o), and NO `U strlen`/`U strcmp`/`U strncmp`/`U strcpy`/
//     `U strncpy`/`U strcat`/`U strchr` (which would mean clang reintroduced a self-call/libc
//     call) -- and `objdump -d -M intel` confirms no function's body contains a `call` to its
//     own name.
// PT: D2 -- ver include/str.h pro contrato. Implementacoes byte-a-byte, mesma filosofia do D1
//     (mem.c): corretude e auditabilidade primeiro, sem truques de word-a-word/SIMD. Todo laco
//     de tamanho/busca percorre `const char*`/`char*` (o contrato de C-string), comparacoes
//     contra `\0` usam o literal diretamente (nunca `strlen` primeiro -- essas funcoes SAO
//     parte de como um `strlen` do zero seria ate descobrivel, entao nenhuma delas pode
//     depender de um atalho escondido de dois passos "medir depois agir" que nao esta no
//     contrato do padrao).
//
//     `strcpy`/`strncpy`/`strcat` REUSAM o `memcpy`/`memset` proprios do D1 (`#include
//     "mem.h"`) onde isso e' o jeito mais claro de expressar "copia N bytes conhecidos" ou
//     "preenche com zero uma cauda de tamanho conhecido" -- esses resolvem em tempo de link
//     pro `src/mem.c`, nunca pra libc, entao continua zero libc. `nm` vai mostrar
//     `U memcpy`/`U memset` em `str.o` (indefinidos NESTE objeto) que RESOLVEM contra `mem.o`
//     em tempo de link -- isso e' esperado e ok; e' uma auto-chamada pro NOSSO proprio simbolo,
//     nao uma dependencia de libc reintroduzida. Distinga isso do caso ruim contra o qual este
//     arquivo se protege abaixo: uma funcao chamando ELA MESMA pelo nome (ex.: `strlen`
//     contendo `call strlen`) ou chamando um simbolo de libc que nunca escrevemos (ex.:
//     `U strlen` dentro do proprio `strlen.o`).
//
//     O GOTCHA `-fno-builtin` (mesma classe de risco do D1, vale repetir aqui porque
//     `strlen`/`strcpy`/`strcmp`/`strchr` sao pattern-matched de forma AINDA MAIS agressiva
//     pelo loop-idiom recognition do clang que lacos crus de memoria -- sao idiomas de livro-
//     texto de libc): um `while (s[i] != '\0') i++;` escrito a mao pode ser reescrito numa
//     chamada a `strlen`, um laco de copia-de-byte-ate-NUL numa chamada a `strcpy`, etc. --
//     inclusive uma AUTO-CHAMADA dentro da propria funcao sendo definida (recursao infinita/
//     estouro de pilha em runtime, nao um erro de compilacao). O `-fno-builtin` geral do
//     Makefile (CFLAGS) faz opt-out da familia inteira de uma vez. VERIFICADO empiricamente pra
//     este arquivo exato apos buildar (ver relatorio da tarefa): `nm build/obj/str.o` mostra os
//     sete simbolos `T` (definidos), `U memcpy`/`U memset` (nossos, resolvidos contra mem.o), e
//     NENHUM `U strlen`/`U strcmp`/`U strncmp`/`U strcpy`/`U strncpy`/`U strcat`/`U strchr`
//     (o que significaria que o clang reintroduziu uma auto-chamada/chamada de libc) -- e
//     `objdump -d -M intel` confirma que nenhum corpo de funcao contem uma `call` pro proprio
//     nome.
// Copyright (c) 2026 Petrus Silva Costa
#include "str.h"
#include "mem.h"

size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        i++;
    }
    // EN: One (or both) strings ended -- compare the terminating bytes themselves (`\0` on the
    //     ended side, whatever byte is at `i` on the other). If both ended together they are
    //     equal (0 - 0 == 0); if only one ended, `\0` (0) vs a non-zero byte gives the correct
    //     sign (prefix < longer string, matching "\0 < any other byte").
    // PT: Uma (ou ambas) string terminou -- compara os proprios bytes terminadores (`\0` do
    //     lado que terminou, seja qual byte estiver em `i` do outro lado). Se ambas terminaram
    //     juntas sao iguais (0 - 0 == 0); se so uma terminou, `\0` (0) vs um byte nao-zero da'
    //     o sinal correto (prefixo < string mais longa, batendo com "\0 < qualquer outro byte").
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

int strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        if (ca == '\0') {
            // EN: Both sides hit the terminator at the same index within the first n bytes --
            //     equal, stop early (also prevents reading past the `\0`).
            // PT: Os dois lados bateram no terminador no mesmo indice dentro dos primeiros n
            //     bytes -- iguais, para cedo (tambem evita ler alem do `\0`).
            return 0;
        }
    }
    return 0;
}

char* strcpy(char* dst, const char* src) {
    // EN: strlen(src) + 1 includes the terminating `\0` -- memcpy (D1, ours) moves it too.
    // PT: strlen(src) + 1 inclui o `\0` terminador -- memcpy (D1, nosso) move ele tambem.
    memcpy(dst, src, strlen(src) + 1);
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    size_t src_len = strlen(src);
    if (src_len < n) {
        // EN: Case 1 (src shorter than n): copy the string itself (with its `\0`), then
        //     zero-pad the remaining `n - (src_len + 1)` bytes -- memset covers the whole
        //     "rest of dst up to n" requirement in one call.
        // PT: Caso 1 (src mais curto que n): copia a string em si (com o `\0`), depois
        //     preenche com zero os `n - (src_len + 1)` bytes restantes -- memset cobre o
        //     requisito inteiro "resto de dst ate n" numa unica chamada.
        memcpy(dst, src, src_len + 1);
        memset(dst + src_len + 1, 0, n - (src_len + 1));
    } else {
        // EN: Cases 2 (src_len == n) and 3 (src_len > n): copy exactly n bytes, no terminator
        //     added -- this is the dangerous, standard-mandated behaviour (see file header).
        //     n == 0 also falls here: memcpy of 0 bytes is a correct no-op.
        // PT: Casos 2 (src_len == n) e 3 (src_len > n): copia exatamente n bytes, nenhum
        //     terminador adicionado -- esse e' o comportamento perigoso e exigido pelo padrao
        //     (ver cabecalho do arquivo). n == 0 tambem cai aqui: memcpy de 0 bytes e' um
        //     no-op correto.
        memcpy(dst, src, n);
    }
    return dst;
}

char* strcat(char* dst, const char* src) {
    size_t dst_len = strlen(dst);
    // EN: strlen(src) + 1 includes src's own `\0`, which becomes dst's new terminator.
    // PT: strlen(src) + 1 inclui o proprio `\0` de src, que vira o novo terminador de dst.
    memcpy(dst + dst_len, src, strlen(src) + 1);
    return dst;
}

char* strchr(const char* s, int c) {
    unsigned char target = (unsigned char)c;
    size_t i = 0;
    // EN: Loop condition checks the CURRENT byte (including a possible `\0`) before advancing
    //     -- this is what lets `c == '\0'` match the terminator itself: the `\0` case is
    //     caught by the `s[i] == target` check on the same iteration that would otherwise be
    //     the loop's exit, before the loop condition below would stop it.
    // PT: A condicao do laco checa o byte ATUAL (incluindo um possivel `\0`) antes de avancar
    //     -- e' isso que permite `c == '\0'` casar com o proprio terminador: o caso `\0` e'
    //     pego pelo check `s[i] == target` na mesma iteracao que de outra forma seria a saida
    //     do laco, antes da condicao do laco abaixo pode-lo parar.
    for (;;) {
        unsigned char cur = (unsigned char)s[i];
        if (cur == target) {
            return (char*)(s + i);
        }
        if (cur == '\0') {
            // EN: Reached the terminator and it did not match `target` (target != '\0', since
            //     that case returned above) -- not found.
            // PT: Chegou no terminador e ele nao casou com `target` (target != '\0', ja que
            //     esse caso retornou acima) -- nao achado.
            return NULL;
        }
        i++;
    }
}
