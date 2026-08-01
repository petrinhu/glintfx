// SPDX-License-Identifier: Apache-2.0
// EN: font_nothrow_sanity (FONT-NOTHROW, W22, 2026-07-30) -- the FOURTH and LAST member of the
//     `never a crash` family (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`, `image_decode_hardening_
//     sanity.cpp`/`image_encode_hardening_sanity.cpp`/`draw2d_texture_nothrow_sanity.cpp`): proves
//     `Draw2d::load_font()` (`draw2d.cpp`) degrades to a clean, default-constructed `Font2d{}`
//     (`ok() == false`) under a REAL, forced `std::bad_alloc`, never a crash -- the SAME technique
//     `draw2d_texture_nothrow_sanity.cpp` already established for `load_texture()`/
//     `create_texture()`: SIZE-MATCHED `operator new` fault injection, not `fork()` + `setrlimit
//     (RLIMIT_AS, ...)` (see that file's own top comment for the measured, empirical reason
//     RLIMIT_AS was tried first and discarded against a live GL/RmlUi process -- Mesa/llvmpipe's
//     own ~2.5 GiB `VmSize` baseline swallows or mis-times the squeeze; the same reasoning applies
//     unchanged here, `Draw2d::init()` needs the SAME real, current GL context `load_font()`
//     itself does not touch but `Draw2d` as a whole requires to be `initialized`).
//
//     THE WRINKLE THIS FILE ADDS ON TOP OF THAT TECHNIQUE, DECLARED UP FRONT: unlike
//     `image_decode_hardening_sanity.cpp`'s own Group A (DEC-MOVE, two DIFFERENT-sized copies --
//     the stb intermediate buffer vs. the RGBA8 output) or `draw2d_texture_nothrow_sanity.cpp`'s
//     own Group L/Group C (two INDEPENDENT allocations, each with its own distinct byte count),
//     `load_font()`'s own two enumerated allocation-bearing calls -- `buf(len)` (`draw2d.cpp`, the
//     file-read buffer) and `TextFace::open()`'s own internal `face.blob_.assign(blob, blob +
//     len)` (`text_raster.cpp`) -- request the EXACT SAME byte count, because the second is a
//     byte-for-byte COPY of the first (`TextFace` OWNS a private copy of the font blob for its own
//     lifetime, see that class' own doc-comment in `text_raster.hpp`, the identical shape
//     `decode_image_file()`'s own DEC-NOTHROW already covers for the image-decode sibling). A
//     plain "throw on first match of this size" oracle can therefore only ever reach the FIRST of
//     the two (`buf(len)`) -- arming for that size fires immediately, before `TextFace::open()` is
//     ever called, so the second allocation site would go permanently unexercised by size alone.
//     FIX: an OCCURRENCE COUNT (`g_skip_remaining`) layered on top of the existing size match --
//     the SAME single-shot, size-matched `operator new` override
//     `draw2d_texture_nothrow_sanity.cpp` already established, extended to let the caller specify
//     "arm, but pass through the first N size-matching allocations before firing on the (N+1)th".
//     Group F1 below asks for the 1st (N=0, catches `buf(len)` itself); Group F2 asks for the 2nd
//     (N=1, catches `TextFace::open()`'s own copy) -- a NEW, deterministic technique, not a
//     calibration-based guess (this file computes `target_size` ANALYTICALLY, from the real
//     fixture file's own on-disk byte count, read via a plain `std::fopen`/`fseek`/`ftell` BEFORE
//     any child is forked -- no calibration child, no measurement, no margin, no run-to-run
//     variance, matching the discipline `draw2d_texture_nothrow_sanity.cpp`'s own top comment
//     already states for its own `target_size`).
//
//     Two independent groups, ONE fixture file shared between them (`assets/OpenSans-Regular.ttf`,
//     already copied to this build's `WORKING_DIRECTORY` by an earlier CMake block this suite's
//     own `fontface_load_sanity.cpp`/`app_draw2d_smoke.cpp` already rely on -- a REAL, valid SFNT/
//     TrueType font, not a hand-rolled byte-for-byte construction the way
//     `image_decode_hardening_sanity.cpp`'s own Group B builds a paletted PNG: `load_font()`'s
//     UNARMED sanity call (see `run_injection_child()` below) must return `ok() == true`, which a
//     hand-rolled minimal SFNT would need to satisfy the sovereign C core's own hostile-input-
//     hardened parser -- reusing an already-proven-valid fixture sidesteps that entirely):
//       Group F1 (`buf(len)`, `draw2d.cpp`'s own file-read buffer, N=0 -- the FIRST occurrence of
//       the fixture's exact on-disk byte count) -- `Draw2d::load_font()`'s OWN direct allocation,
//       mirroring `draw2d_texture_nothrow_sanity.cpp`'s own Group L/Group C shape most directly.
//       Group F2 (`TextFace::open()`'s own `blob_.assign()`, `text_raster.cpp`, N=1 -- the SECOND
//       occurrence of that same byte count) -- the SAME shape of risk `DEC-MOVE` maps for the
//       image-decode sibling: an allocation happening INSIDE a function `load_font()` calls into,
//       not at `load_font()`'s own call site, still inside the SAME `try`/`catch` (`draw2d.cpp`'s
//       own FONT-NOTHROW top comment on `load_font()` states why ONE function-wide `try`, not
//       several call-scoped ones, same rationale as `load_texture()`'s own TEX-NOTHROW).
//
//     `fork()` is still used (one child per group), but ONLY for crash-isolation and clean
//     reporting -- an uncaught exception inside the SAME process as this test's own `main()` would
//     abort the WHOLE binary before any later group's `check()`s could run; a forked child that
//     SIGABRTs instead reports cleanly via `waitpid()`'s own `WIFSIGNALED()`, exactly like every
//     other fork-based oracle in this suite. Each child stands up its OWN `WindowGlfw`+`Draw2d`+
//     `init()` (the parent never touches GL/GLFW/X11) -- required because `load_font()` itself
//     guards on `impl_->initialized` (`draw2d.cpp`) even though it never reaches GL past that
//     guard, the SAME "requires a live init()ed instance" contract `load_texture()` has.
//
//     DECLARED COVERAGE GAP (do not "fix" this by widening the injection without reading this
//     first -- SAME discipline `draw2d_texture_nothrow_sanity.cpp`'s own declared gap already
//     established for its `push_back()`): `impl_->fonts.push_back(Impl::FontSlot{})` (the
//     registry-growth allocation, `draw2d.cpp`, reached AFTER both targeted allocations succeed)
//     and every `impl_->log_warn()` string-concatenation call on the guard-clause branches (reached
//     BEFORE either targeted allocation, on hostile/malformed input this file does not construct)
//     sit inside the SAME `try`/`catch` and are therefore protected BY CONSTRUCTION -- the
//     whole-body mutation test this file's own delivery notes describe proves this (removing the
//     `try` drops EVERYTHING inside it) -- but neither is EXERCISED-AND-OBSERVED the way the two
//     targeted allocations are. Deliberate, for the SAME reason `draw2d_texture_nothrow_sanity.cpp`
//     gives: the exact byte count a `std::vector<Impl::FontSlot>` growth or a `std::string`
//     concatenation allocates is an implementation detail of libstdc++'s own growth/SSO policy, not
//     part of this library's own contract -- a THIRD/FOURTH injection target pinned to those
//     numbers would be a fragile oracle coupled to a toolchain detail. Declared, structural
//     coverage here beats a silently fragile, "more thorough-looking" one.
//
//     MUTATION-TESTED (house discipline, `feedback_auditoria_domino`): the `try`/`catch` was
//     removed from `load_font()` (`draw2d.cpp`) with the real fix already committed BEFORE the
//     mutation, restored via a plain `Edit` (never `git checkout --` on a not-yet-committed file --
//     `feedback_mutante_em_arquivo_nao_commitado`) and REBUILT after restoring (never trusting a
//     stale binary -- same lesson `ENC-NOTHROW`'s own delivery notes record). See this slice's own
//     delivery notes (`/var/tmp/w22_font_nothrow_notas.md`) for the exact procedure and the SIGABRT/
//     PASS evidence both directions.
//
//     Linux-only in the sense every other GL-context'd test in this suite already is (`WindowGlfw`
//     under Xvfb, `fork()`) -- matches this whole library's own "Linux x86-64" platform scope
//     (`CLAUDE.md`). Registered for `GLINTFX_MODULE_DRAW2D` in BOTH build configurations (Draw2D
//     never depends on `GLINTFX_BACKEND_GLFW` -- ADR-0017 axis 1), same `${_embed_dep}` link pair
//     every other `draw2d_*` test in this suite uses -- `WindowGlfw` is available as a hidden
//     GL-context host regardless of the backend flag.
// PT: font_nothrow_sanity (FONT-NOTHROW, W22, 2026-07-30) -- o QUARTO e ÚLTIMO membro da família
//     `never a crash` (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`,
//     `image_decode_hardening_sanity.cpp`/`image_encode_hardening_sanity.cpp`/
//     `draw2d_texture_nothrow_sanity.cpp`): prova que `Draw2d::load_font()` (`draw2d.cpp`) degrada
//     pra um `Font2d{}` limpo, default-construído (`ok() == false`) sob um `std::bad_alloc` REAL,
//     forçado, nunca um crash -- a MESMA técnica que `draw2d_texture_nothrow_sanity.cpp` já
//     estabeleceu pro `load_texture()`/`create_texture()`: injeção de falha em `operator new`
//     CASADA POR TAMANHO, não `fork()` + `setrlimit(RLIMIT_AS, ...)` (ver o próprio comentário de
//     topo daquele arquivo pro motivo medido, empírico, de o RLIMIT_AS ter sido tentado primeiro e
//     descartado contra um processo GL/RmlUi vivo -- o próprio `VmSize` baseline de ~2,5 GiB do
//     Mesa/llvmpipe engole ou desacerta o aperto; o mesmo raciocínio se aplica sem mudança aqui, o
//     `Draw2d::init()` precisa do MESMO contexto GL real que o próprio `load_font()` não toca mas
//     que o `Draw2d` como um todo exige pra estar `initialized`).
//
//     A RUGA QUE ESTE ARQUIVO SOMA EM CIMA DAQUELA TÉCNICA, DECLARADA DE ANTEMÃO: diferente do
//     próprio Grupo A (DEC-MOVE, duas cópias de tamanho DIFERENTE -- o buffer intermediário do stb
//     vs. a saída RGBA8) de `image_decode_hardening_sanity.cpp` ou do próprio Grupo L/Grupo C
//     (duas alocações INDEPENDENTES, cada uma com a própria contagem de bytes distinta) de
//     `draw2d_texture_nothrow_sanity.cpp`, as duas próprias chamadas portadoras-de-alocação
//     enumeradas do `load_font()` -- `buf(len)` (`draw2d.cpp`, o buffer de leitura de arquivo) e o
//     próprio `face.blob_.assign(blob, blob + len)` interno do `TextFace::open()`
//     (`text_raster.cpp`) -- pedem a EXATA MESMA contagem de bytes, porque a segunda é uma CÓPIA
//     byte-a-byte da primeira (o TextFace POSSUI uma cópia privada do blob de fonte pela própria
//     vida, ver o doc-comment próprio daquela classe em `text_raster.hpp`, a MESMA forma que o
//     próprio DEC-NOTHROW do `decode_image_file()` já cobre pro irmão de decode de imagem). Um
//     oráculo simples "lança no primeiro casamento deste tamanho" só consegue então alcançar a
//     PRIMEIRA das duas (`buf(len)`) -- armar pra aquele tamanho dispara imediatamente, antes do
//     `TextFace::open()` sequer ser chamado, então o segundo sítio de alocação ficaria permanente e
//     silenciosamente sem exercício só por tamanho. CONSERTO: uma CONTAGEM DE OCORRÊNCIA
//     (`g_skip_remaining`) sobreposta ao próprio casamento por tamanho já existente -- o MESMO
//     override de disparo único, casado por tamanho, de `operator new` que
//     `draw2d_texture_nothrow_sanity.cpp` já estabeleceu, estendido pra deixar quem chama
//     especificar "arma, mas deixa passar as primeiras N alocações casantes-de-tamanho antes de
//     disparar na (N+1)ésima". O Grupo F1 abaixo pede a 1ª (N=0, pega o próprio `buf(len)`); o
//     Grupo F2 pede a 2ª (N=1, pega a própria cópia do `TextFace::open()`) -- uma técnica NOVA,
//     determinística, não um chute baseado em calibração (este arquivo computa `target_size`
//     ANALITICAMENTE, a partir da própria contagem de bytes em disco do fixture real, lida via um
//     `std::fopen`/`fseek`/`ftell` puro ANTES de qualquer filho ser forkado -- nenhum filho de
//     calibração, nenhuma medição, nenhuma margem, nenhuma variância execução-a-execução, batendo
//     com a disciplina que o próprio comentário de topo de `draw2d_texture_nothrow_sanity.cpp` já
//     afirma pro próprio `target_size`).
//
//     Dois grupos independentes, UM fixture compartilhado entre eles (`assets/OpenSans-Regular.ttf`,
//     já copiado pro `WORKING_DIRECTORY` deste build por um bloco CMake anterior do qual o próprio
//     `fontface_load_sanity.cpp`/`app_draw2d_smoke.cpp` desta suíte já dependem -- uma fonte SFNT/
//     TrueType REAL, válida, não uma construção byte-a-byte feita à mão do jeito que o próprio
//     Grupo B de `image_decode_hardening_sanity.cpp` constrói um PNG paletizado: a chamada de
//     sanidade DESARMADA do `load_font()` (ver `run_injection_child()` abaixo) precisa retornar
//     `ok() == true`, o que uma SFNT mínima feita à mão precisaria satisfazer contra o próprio
//     parser endurecido-contra-input-hostil do núcleo C soberano -- reusar um fixture já-provado-
//     válido evita isso inteiramente):
//       Grupo F1 (`buf(len)`, o próprio buffer de leitura de arquivo de `draw2d.cpp`, N=0 -- a
//       PRIMEIRA ocorrência da contagem de bytes em disco exata do fixture) -- a própria alocação
//       DIRETA do `Draw2d::load_font()`, espelhando a forma do próprio Grupo L/Grupo C de
//       `draw2d_texture_nothrow_sanity.cpp` mais diretamente.
//       Grupo F2 (o próprio `blob_.assign()` do `TextFace::open()`, `text_raster.cpp`, N=1 -- a
//       SEGUNDA ocorrência daquela mesma contagem de bytes) -- a MESMA forma de risco que o
//       `DEC-MOVE` mapeia pro irmão de decode de imagem: uma alocação acontecendo DENTRO de uma
//       função que o `load_font()` chama, não no próprio sítio de chamada do `load_font()`, ainda
//       dentro do MESMO `try`/`catch` (o próprio comentário de topo FONT-NOTHROW do `load_font()`
//       em `draw2d.cpp` afirma o porquê de UM try do tamanho da função, não vários escopados por
//       chamada, mesmo racional do próprio TEX-NOTHROW do `load_texture()`).
//
//     `fork()` ainda é usado (um filho por grupo), mas SÓ pra isolamento-de-crash e relato limpo --
//     uma exceção não-capturada dentro do MESMO processo do próprio `main()` deste teste abortaria
//     o binário INTEIRO antes que os `check()`s de qualquer grupo posterior pudessem rodar; um
//     filho forkado que sofre SIGABRT em vez disso relata de forma limpa via o próprio
//     `WIFSIGNALED()` do `waitpid()`, exatamente como todo outro oráculo baseado em fork desta
//     suíte. Cada filho monta o PRÓPRIO `WindowGlfw`+`Draw2d`+`init()` (o pai nunca toca GL/GLFW/
//     X11) -- necessário porque o próprio `load_font()` guarda em `impl_->initialized`
//     (`draw2d.cpp`) mesmo nunca alcançando GL depois daquela guarda, o MESMO contrato "exige uma
//     instância init()ada viva" que o `load_texture()` tem.
//
//     LACUNA DE COBERTURA DECLARADA (não "conserte" isto alargando a injeção sem ler primeiro --
//     MESMA disciplina que a própria lacuna declarada de `draw2d_texture_nothrow_sanity.cpp` já
//     estabeleceu pro `push_back()` dela): o `impl_->fonts.push_back(Impl::FontSlot{})` (a alocação
//     de crescimento do registry, `draw2d.cpp`, alcançada DEPOIS de ambas as alocações miradas
//     terem sucesso) e toda chamada de concatenação de string do `impl_->log_warn()` nos ramos de
//     guarda (alcançada ANTES de qualquer uma das duas alocações miradas, em input hostil/malformado
//     que este arquivo não constrói) ficam dentro do MESMO `try`/`catch` e por isso são protegidos
//     POR CONSTRUÇÃO -- o teste de mutação de corpo inteiro que as próprias notas de entrega deste
//     arquivo descrevem prova isso (remover o `try` derruba TUDO que está dentro) -- mas nenhum dos
//     dois é EXERCITADO-E-OBSERVADO do jeito que as duas alocações miradas são. Deliberado, pelo
//     MESMO motivo que o próprio `draw2d_texture_nothrow_sanity.cpp` dá: a contagem exata de bytes
//     que um crescimento de `std::vector<Impl::FontSlot>` ou uma concatenação de `std::string`
//     aloca é um detalhe de implementação da própria política de crescimento/SSO do libstdc++, não
//     parte do próprio contrato desta biblioteca -- um TERCEIRO/QUARTO alvo de injeção fixado
//     naqueles números seria um oráculo frágil acoplado a um detalhe de toolchain. Cobertura
//     estrutural declarada aqui vale mais que uma "de aparência mais completa" mas silenciosamente
//     frágil.
//
//     TESTADO POR MUTAÇÃO (disciplina da casa, `feedback_auditoria_domino`): o `try`/`catch` foi
//     removido do `load_font()` (`draw2d.cpp`) com o conserto real já commitado ANTES da mutação,
//     restaurado via um `Edit` puro (nunca `git checkout --` num arquivo ainda-não-commitado --
//     `feedback_mutante_em_arquivo_nao_commitado`) e REBUILDADO depois de restaurar (nunca
//     confiando num binário obsoleto -- a MESMA lição que as próprias notas de entrega do
//     `ENC-NOTHROW` registram). Ver as próprias notas de entrega desta fatia
//     (`/var/tmp/w22_font_nothrow_notas.md`) pro procedimento exato e a evidência SIGABRT/PASS nas
//     duas direções.
//
//     Só Linux no sentido em que todo outro teste com contexto GL desta suíte já é (`WindowGlfw`
//     sob Xvfb, `fork()`) -- bate com o próprio escopo de plataforma "Linux x86-64" de toda esta
//     biblioteca (`CLAUDE.md`). Registrado pro `GLINTFX_MODULE_DRAW2D` nas DUAS configurações de
//     build (o Draw2D nunca depende de `GLINTFX_BACKEND_GLFW` -- ADR-0017 eixo 1), o MESMO par de
//     link `${_embed_dep}` que todo outro teste `draw2d_*` desta suíte usa -- `WindowGlfw` está
//     disponível como host oculto de contexto GL independente da flag de backend.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

#include <sys/wait.h>
#include <unistd.h>

using glintfx::Draw2d;
using glintfx::Font2d;
using glintfx::WindowGlfw;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Single-shot, size-matched, OCCURRENCE-COUNTED fault injection -- see this file's own top
//     comment for the full rationale (why an occurrence count is layered on top of
//     `draw2d_texture_nothrow_sanity.cpp`'s own size-matched technique here specifically).
//     `g_armed` gates the override so ONLY deliberately-targeted allocations are ever affected;
//     `g_target_size` is the exact byte count to match; `g_skip_remaining` counts down on each
//     size-matching allocation BEFORE the one that actually throws -- a match while
//     `g_skip_remaining > 0` is let through normally (decrementing the counter), a match once it
//     reaches `0` disarms itself (`g_armed = false`) and throws, so a caller that somehow retries
//     the same allocation size inside its own exception handling never gets hit twice.
// PT: Injeção de falha de disparo único, casada por tamanho, com CONTAGEM DE OCORRÊNCIA -- ver o
//     próprio comentário de topo deste arquivo pro racional completo (por que uma contagem de
//     ocorrência é sobreposta à própria técnica casada-por-tamanho de
//     `draw2d_texture_nothrow_sanity.cpp` aqui especificamente). `g_armed` restringe o override pra
//     que SÓ alocações deliberadamente miradas sejam algum dia afetadas; `g_target_size` é a
//     contagem exata de bytes a casar; `g_skip_remaining` conta regressivamente a cada alocação
//     casante-de-tamanho ANTES da que de fato lança -- um casamento enquanto `g_skip_remaining > 0`
//     é deixado passar normalmente (decrementando o contador), um casamento quando chega em `0` se
//     desarma (`g_armed = false`) e lança, pra que um chamador que de alguma forma tente de novo o
//     mesmo tamanho de alocação dentro do próprio tratamento de exceção nunca seja atingido duas
//     vezes.
bool g_armed = false;
std::size_t g_target_size = 0;
int g_skip_remaining = 0;

} // namespace

void* operator new(std::size_t sz) {
  if (g_armed && sz == g_target_size) {
    if (g_skip_remaining > 0) {
      --g_skip_remaining;
    } else {
      g_armed = false;
      throw std::bad_alloc();
    }
  }
  void* p = std::malloc(sz);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

struct ChildOutcome {
  bool spawned = false;
  bool signalled = false;
  int term_signal = 0;
  bool exited = false;
  int exit_code = -1;
};

// EN: One forked child per group. Stands up the child's OWN WindowGlfw+Draw2d+init() (the PARENT
//     never touches GL/GLFW/X11) and runs an UNARMED sanity call of `load_font(font_path)`, which
//     MUST succeed (`ok() == true` -- proves the fixture itself parses cleanly before the armed
//     call below is trusted to mean anything). Then arms `target_size`/`skip_count` and calls
//     `load_font(font_path)` a SECOND time -- the injected `std::bad_alloc` must strike inside that
//     second call, at the (skip_count+1)th allocation of `target_size` bytes, and `load_font()`
//     must degrade to `ok() == false` without crashing. Exit codes: 4 = window create failed,
//     5 = Draw2d::init() failed (both SKIPPED, inconclusive), 6 = the UNARMED sanity call itself
//     failed (a genuine FAIL -- the fixture is broken), 0 = the armed call correctly reported
//     ok()==false, 1 = the armed call unexpectedly reported ok()==true (the injected throw never
//     landed -- a genuine FAIL, not a skip).
// PT: Um filho forkado por grupo. Monta o PRÓPRIO WindowGlfw+Draw2d+init() do filho (o PAI nunca
//     toca GL/GLFW/X11) e roda uma chamada de sanidade DESARMADA de `load_font(font_path)`, que
//     PRECISA ter sucesso (`ok() == true` -- prova que o próprio fixture parseia limpo antes da
//     chamada armada abaixo ser confiada a significar algo). Então arma `target_size`/`skip_count`
//     e chama `load_font(font_path)` uma SEGUNDA vez -- o `std::bad_alloc` injetado precisa atingir
//     dentro daquela segunda chamada, na (skip_count+1)ésima alocação de `target_size` bytes, e o
//     `load_font()` precisa degradar pra `ok() == false` sem crashar. Códigos de saída:
//     4 = criação de janela falhou, 5 = Draw2d::init() falhou (ambos PULADOS, inconclusivo),
//     6 = a PRÓPRIA chamada de sanidade DESARMADA falhou (uma FALHA genuína -- o fixture está
//     quebrado), 0 = a chamada armada reportou corretamente ok()==false, 1 = a chamada armada
//     reportou inesperadamente ok()==true (o lançamento injetado nunca chegou -- uma FALHA
//     genuína, não um pulo).
ChildOutcome run_injection_child(const char* font_path, std::size_t target_size, int skip_count) {
  ChildOutcome out;
  const pid_t pid = fork();
  if (pid < 0) return out;
  out.spawned = true;

  if (pid == 0) {
    WindowGlfw window;
    if (!window.create("font_nothrow_child", 64, 64)) _exit(4);
    Draw2d d2d;
    if (!d2d.init()) _exit(5);

    // EN: Sanity: unarmed, must succeed -- proves the fixture is valid before the armed call
    //     below is trusted.
    // PT: Sanidade: desarmada, precisa ter sucesso -- prova que a fixture é válida antes da
    //     chamada armada abaixo ser confiada.
    if (!d2d.load_font(font_path).ok()) _exit(6);

    // EN: THE injected failure. If FONT-NOTHROW regresses (the try/catch removed, or the
    //     allocation genuinely escapes uncaught), std::bad_alloc propagates uncaught ->
    //     std::terminate() -> abort() -> this process dies by SIGABRT, and the parent's
    //     WIFSIGNALED() check below catches it.
    // PT: A falha injetada. Se o FONT-NOTHROW regredir (o try/catch removido, ou a alocação
    //     genuinamente escapar não-capturada), std::bad_alloc se propaga não-capturada ->
    //     std::terminate() -> abort() -> este processo morre por SIGABRT, e a checagem
    //     WIFSIGNALED() do pai abaixo pega isso.
    g_target_size = target_size;
    g_skip_remaining = skip_count;
    g_armed = true;
    const bool ok = d2d.load_font(font_path).ok();
    g_armed = false; // defensive: the call may have returned without ever touching target_size.

    _exit(ok ? 1 : 0);
  }

  int status = 0;
  waitpid(pid, &status, 0);
  out.signalled = WIFSIGNALED(status);
  out.term_signal = out.signalled ? WTERMSIG(status) : 0;
  out.exited = WIFEXITED(status);
  out.exit_code = out.exited ? WEXITSTATUS(status) : -1;
  return out;
}

// EN: Applies the PASS/SKIP/FAIL bucketing described on run_injection_child()'s own doc-comment.
//     `label` prefixes every `check()` message so a failure names its own group unambiguously.
// PT: Aplica a classificação PASSA/PULA/FALHA descrita no próprio doc-comment de
//     run_injection_child(). `label` prefixa toda mensagem de `check()` pra que uma falha nomeie o
//     próprio grupo sem ambiguidade.
void run_injection_oracle(const char* label, const char* font_path, std::size_t target_size,
                          int skip_count) {
  char msg[256];

  const ChildOutcome r = run_injection_child(font_path, target_size, skip_count);
  std::snprintf(msg, sizeof(msg), "%s: child's fork() succeeded", label);
  check(r.spawned, msg);

  std::snprintf(msg, sizeof(msg),
                "%s (FONT-NOTHROW): child was NOT killed by a signal (uncaught std::bad_alloc "
                "would SIGABRT) -- WIFSIGNALED=%d, signal=%d",
                label, r.signalled ? 1 : 0, r.signalled ? r.term_signal : 0);
  check(!r.signalled, msg);

  if (r.signalled || !r.exited) return;

  if (r.exit_code == 4) {
    std::fprintf(stderr, "%s: SKIPPED -- child's window create failed -- inconclusive\n", label);
  } else if (r.exit_code == 5) {
    std::fprintf(stderr, "%s: SKIPPED -- child's Draw2d::init() failed -- inconclusive\n", label);
  } else if (r.exit_code == 6) {
    std::snprintf(msg, sizeof(msg),
                  "%s setup: the UNARMED sanity call succeeded (fixture itself is valid)", label);
    check(false, msg);
  } else {
    std::snprintf(msg, sizeof(msg),
                  "%s: the armed call degraded to ok()==false under the injected, size-matched "
                  "allocation failure (exit 0 expected; 1 means the injected throw never landed)",
                  label);
    check(r.exit_code == 0, msg);
  }
}

} // namespace

int main() {
  // EN: Fixture: the SAME real, valid SFNT/TrueType font this suite's own fontface_load_sanity.cpp/
  //     app_draw2d_smoke.cpp already rely on (see this file's own top comment for why -- a
  //     hand-rolled minimal SFNT is not needed here, and would need to satisfy the sovereign C
  //     core's own hostile-input-hardened parser to be worth building at all). The exact on-disk
  //     byte count -- read ONCE, here, BEFORE any child is forked -- IS `target_size` for both
  //     groups below (see this file's own top comment for why the two allocations under test
  //     coincide in size).
  // PT: Fixture: a MESMA fonte SFNT/TrueType real, válida, da qual o próprio
  //     fontface_load_sanity.cpp/app_draw2d_smoke.cpp desta suíte já dependem (ver o próprio
  //     comentário de topo deste arquivo pro porquê -- uma SFNT mínima feita à mão não é necessária
  //     aqui, e precisaria satisfazer o próprio parser endurecido-contra-input-hostil do núcleo C
  //     soberano pra sequer valer a pena construir). A contagem exata de bytes em disco -- lida UMA
  //     vez, aqui, ANTES de qualquer filho ser forkado -- É o `target_size` pros dois grupos
  //     abaixo (ver o próprio comentário de topo deste arquivo pro porquê das duas alocações sob
  //     teste coincidirem em tamanho).
  const char* kFontPath = "assets/OpenSans-Regular.ttf";

  std::size_t file_size = 0;
  {
    std::FILE* f = std::fopen(kFontPath, "rb");
    check(f != nullptr, "setup: fixture font file assets/OpenSans-Regular.ttf opened");
    if (f) {
      const bool seek_ok = std::fseek(f, 0, SEEK_END) == 0;
      check(seek_ok, "setup: fixture font file seek-to-end succeeded");
      if (seek_ok) {
        const long sz = std::ftell(f);
        check(sz > 0, "setup: fixture font file size is positive");
        if (sz > 0) file_size = static_cast<std::size_t>(sz);
      }
      std::fclose(f);
    }
  }

  if (file_size == 0) {
    std::fprintf(stderr,
                 "font_nothrow_sanity: fixture font file missing or empty -- aborting, see "
                 "check() failures above\n");
    return 1;
  }

  // ===========================================================================================
  // Group F1: the 1st occurrence of `file_size` bytes -- Draw2d::load_font()'s own `buf(len)`
  // file-read buffer (draw2d.cpp).
  // ===========================================================================================
  run_injection_oracle("Group F1 (load_font file-read buffer)", kFontPath, file_size, 0);

  // ===========================================================================================
  // Group F2: the 2nd occurrence of `file_size` bytes -- TextFace::open()'s own internal
  // `blob_.assign()` SECOND copy of the same bytes (text_raster.cpp).
  // ===========================================================================================
  run_injection_oracle("Group F2 (TextFace::open() second copy)", kFontPath, file_size, 1);

  if (g_failures > 0) {
    std::fprintf(stderr, "font_nothrow_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("font_nothrow_sanity: PASS");
  return 0;
}
