// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_texture_nothrow_sanity (TEX-NOTHROW, W21, 2026-07-30) -- consumer-driven
//     auditoria-dominó follow-up to `image_decode_hardening_sanity.cpp`'s own DEC-NOTHROW/DEC-MOVE
//     (`src/image.cpp`): that fix covered the PUBLIC `decode_image_file()`/`decode_image_memory()`
//     pair; the reviewer's own reproduction (`ulimit -v 950000`, a 12000x12000 paletted PNG) also
//     escapes through `Draw2d::load_texture()` -- the ACTUAL path a consumer reaches for -- because
//     `decode_premultiplied_rgba()` (`image_decode.hpp`) is the BYTE-FOR-BYTE premultiplied sibling
//     of the `decode_straight_rgba()` DEC-NOTHROW already proved throws `std::bad_alloc` for a
//     real, paletted-source input. `Draw2d::create_texture()`'s own `Rgba8` ingest copy has the
//     SAME shape for a different allocation (a caller-supplied buffer large enough that copying it
//     a second time, for the premultiply step, itself fails). Both methods are now `noexcept`
//     (`draw2d.hpp`), wrapping their own body in a `try`/`catch` (`draw2d.cpp`, see that file's own
//     TEX-NOTHROW top comment on each function) that degrades ANY exception to a clean,
//     default-constructed `Texture2d{}` (`ok() == false`) -- never a crash.
//
//     TARGETED, SIZE-MATCHED `operator new` FAULT INJECTION -- not `fork()` + `setrlimit(RLIMIT_AS,
//     ...)`, the technique `image_decode_hardening_sanity.cpp`'s own Group B uses for the sibling
//     PURE-CPU decode pair. That technique was tried here FIRST and discarded for a measured,
//     empirical reason, not a hunch: `Draw2d::init()` needs a REAL, CURRENT GL context (Mesa/
//     llvmpipe), and a live GL/RmlUi process's own `VmSize` is already enormous (~2.5 GiB measured,
//     virtual address space reserved by Mesa's LLVM JIT and glibc's own per-arena bookkeeping, not
//     resident memory) well BEFORE either method under test ever allocates a single byte. Squeezing
//     `RLIMIT_AS` against that baseline produced TWO distinct failure shapes across five identical
//     runs, neither of them useful: (1) a 20 MiB decode/copy allocation routinely REUSED already-
//     reserved-but-unmapped arena space and never triggered a new `mmap()` at all, so the intended
//     failure never fired (`load_texture()` "unexpectedly" succeeded every time); (2) squeezing the
//     budget tighter to compensate moved the actual failure point PAST the C++ allocation and into
//     Mesa/llvmpipe's own internal texture-storage allocation (`glTexImage2D`, reached right after
//     the C++ step this file means to test) -- a raw `SIGSEGV` in the software rasterizer's own
//     driver code, not a `std::bad_alloc` this file's own `try`/`catch` could ever be expected to
//     catch (GL is a C API; it was never a candidate for the fix this file proves, and starving a
//     THIRD-PARTY driver of memory is not this project's fix to make robust). Real memory pressure
//     against a process this large is simply the wrong tool here.
//
//     The ROBUST alternative, ALREADY established in this very repo by
//     `image_decode_hardening_sanity.cpp`'s own Group A (DEC-MOVE): override the GLOBAL
//     `::operator new`/`::operator delete` for the whole test binary, watch for an allocation of an
//     EXACT, distinctive (non-power-of-two, collision-unlikely) byte count, and -- the one
//     extension this file adds -- when ARMED and that exact size is requested, THROW
//     `std::bad_alloc` immediately instead of delegating to `std::malloc`, then immediately
//     DISARM (single-shot: only the ONE matching allocation this file targets is ever hit, nothing
//     else in the same process). This is not a mock of the function under test: the exception is a
//     REAL `std::bad_alloc`, thrown from the REAL global allocator, at the REAL call site
//     (`decode_premultiplied_rgba()`'s own `resize()` for Group L, `create_texture()`'s own
//     `std::vector<unsigned char> premultiplied(...)` range-construction for Group C) -- exactly
//     what a genuine allocator failure looks like from the calling code's own point of view, with
//     none of the GL-driver blast radius real memory exhaustion carries in a process this large.
//     `target_size` is computed ANALYTICALLY (`w * h * 4`, the exact byte count both allocations
//     under test request, matching libstdc++'s own `std::vector<unsigned char>` allocator, which
//     requests `n * sizeof(unsigned char)` bytes with no rounding of its own) -- no calibration
//     child, no measurement, no margin, no run-to-run variance: the injected failure fires on
//     EVERY run, deterministically, is armed only immediately around the ONE call under test (an
//     unarmed "sanity" call of the SAME `call_under_test` runs first and must succeed normally,
//     proving the fixture itself is valid before the armed call proves the fix), and never touches
//     GL/Mesa at all -- the fault fires and is caught strictly BEFORE either method under test ever
//     reaches its own `glTexImage2D` call, by construction (both allocations happen earlier in the
//     function body than the GL upload -- `draw2d.cpp`'s own TEX-NOTHROW top comment on each
//     function).
//
//     `fork()` is still used (one child per group), but ONLY for crash-isolation and clean
//     reporting -- an uncaught exception inside the SAME process as this test's own `main()` would
//     abort the WHOLE binary before any later group's `check()`s could run; a forked child that
//     SIGABRTs instead reports cleanly via `waitpid()`'s own `WIFSIGNALED()`, exactly like every
//     other fork-based oracle in this suite.
//
//     Two independent groups, one per `noexcept` method, each with its OWN distinctive target size
//     so a stray match in one group's child can never be confused with the other's (though each
//     runs in its own forked child regardless, so cross-group collision was never actually possible
//     -- kept distinct anyway, for an unambiguous DEBUG trace if either group ever needs one):
//       Group L (`load_texture()`) -- `137x101` px (`55348` bytes), the SAME distinctive dimensions
//       `image_decode_hardening_sanity.cpp`'s own Group A already uses and proved collision-safe in
//       THIS codebase (though that proof was in a GL-free process; a fresh, GL-context'd run here
//       re-confirms it). The fixture PNG is built via this library's OWN public
//       `encode_image_memory()` (`glintfx/image.hpp`, IMG-ENCODE/ENC-NOTHROW, W21) -- no hand-rolled
//       PNG/zlib/CRC32 needed (Group L does not need the paletted-source trick
//       `image_decode_hardening_sanity.cpp`'s own Group B relies on to keep stb's OWN internal peak
//       below this library's own copy: THIS file forces the failure directly, by size, not by
//       exhausting a derived memory budget, so any valid, decodable PNG of the right pixel count
//       works).
//       Group C (`create_texture()`) -- `179x151` px (`108116` bytes), no decode/PNG involved at
//       all: a plain, caller-owned `Rgba8` pixel buffer built directly in memory.
//
//     MUTATION-TESTED (house discipline, `feedback_auditoria_domino`): both groups were proven to
//     go RED against a build with the `try`/`catch` removed from the method under test (the armed
//     child SIGABRTs instead of exiting 0) before this file was considered done -- see this slice's
//     own delivery notes for the exact procedure (commit the fix first, mutate the tracked file
//     transiently, rebuild, observe red, `git checkout --` restore, rebuild green again).
//
//     Linux-only in the sense every other GL-context'd test in this suite already is (`WindowGlfw`
//     under Xvfb, `fork()`) -- matches this whole library's own "Linux x86-64" platform scope
//     (`CLAUDE.md`). Registered for `GLINTFX_MODULE_DRAW2D` in BOTH build configurations (Draw2D
//     never depends on `GLINTFX_BACKEND_GLFW` -- ADR-0017 axis 1), same `${_embed_dep}` link pair
//     every other `draw2d_*` test in this suite uses -- `WindowGlfw` is available as a hidden
//     GL-context host regardless of the backend flag.
// PT: draw2d_texture_nothrow_sanity (TEX-NOTHROW, W21, 2026-07-30) -- follow-up de
//     auditoria-dominó consumer-driven do próprio DEC-NOTHROW/DEC-MOVE de
//     `image_decode_hardening_sanity.cpp` (`src/image.cpp`): aquele conserto cobriu o par PÚBLICO
//     `decode_image_file()`/`decode_image_memory()`; a própria reprodução do reviewer
//     (`ulimit -v 950000`, um PNG paletizado 12000x12000) também escapa através do
//     `Draw2d::load_texture()` -- o caminho REAL que um consumidor usa -- porque
//     `decode_premultiplied_rgba()` (`image_decode.hpp`) é o irmão premultiplicado BYTE-A-BYTE do
//     `decode_straight_rgba()` que o DEC-NOTHROW já provou lançar `std::bad_alloc` pra um input
//     real, paletizado. A própria cópia de ingestão `Rgba8` do `Draw2d::create_texture()` tem a
//     MESMA forma pra uma alocação diferente (um buffer fornecido pelo chamador grande o bastante
//     pra que copiá-lo uma segunda vez, pro passo de premultiply, ele mesmo falhe). As duas funções
//     agora são `noexcept` (`draw2d.hpp`), envolvendo o PRÓPRIO corpo num `try`/`catch`
//     (`draw2d.cpp`, ver o próprio comentário de topo TEX-NOTHROW daquele arquivo em cada função)
//     que degrada QUALQUER exceção pra um `Texture2d{}` limpo, default-construído
//     (`ok() == false`) -- nunca um crash.
//
//     INJEÇÃO DE FALHA DIRECIONADA, CASADA POR TAMANHO, em `operator new` -- não `fork()` +
//     `setrlimit(RLIMIT_AS, ...)`, a técnica que o próprio Grupo B de
//     `image_decode_hardening_sanity.cpp` usa pro par de decode PURO-CPU irmão. Aquela técnica foi
//     tentada aqui PRIMEIRO e descartada por um motivo medido, empírico, não um palpite: o
//     `Draw2d::init()` precisa de um contexto GL REAL, CORRENTE (Mesa/llvmpipe), e o próprio
//     `VmSize` de um processo GL/RmlUi vivo já é enorme (~2,5 GiB medido, espaço de endereço
//     virtual reservado pelo JIT LLVM do Mesa e pela própria contabilidade por-arena do glibc, não
//     memória residente) bem ANTES de qualquer um dos dois métodos sob teste sequer alocar um byte
//     sequer. Apertar o `RLIMIT_AS` contra aquela linha de base produziu DUAS formas distintas de
//     falha em cinco execuções idênticas, nenhuma das duas útil: (1) uma alocação de decode/cópia
//     de 20 MiB REUSAVA rotineiramente espaço de arena já reservado-mas-não-mapeado e nunca
//     disparava um `mmap()` novo, então a falha pretendida nunca disparava (`load_texture()`
//     "inesperadamente" tinha sucesso toda vez); (2) apertar o orçamento mais forte pra compensar
//     movia o ponto real de falha PRA DEPOIS da alocação C++ e PRA DENTRO da própria alocação
//     interna de armazenamento de textura do Mesa/llvmpipe (`glTexImage2D`, alcançado logo após o
//     passo C++ que este arquivo pretende testar) -- um `SIGSEGV` cru no próprio código de driver
//     do rasterizador por software, não um `std::bad_alloc` que o próprio `try`/`catch` deste
//     arquivo pudesse algum dia esperar capturar (GL é uma API C; nunca foi candidato ao conserto
//     que este arquivo prova, e esfomear um driver de TERCEIRO não é um conserto deste projeto pra
//     fazer). Pressão de memória real contra um processo deste tamanho é simplesmente a ferramenta
//     errada aqui.
//
//     A alternativa ROBUSTA, JÁ estabelecida neste MESMO repo pelo próprio Grupo A (DEC-MOVE) de
//     `image_decode_hardening_sanity.cpp`: sobrescrever o `::operator new`/`::operator delete`
//     GLOBAL pro binário de teste inteiro, observar uma alocação de uma contagem de bytes EXATA,
//     distintiva (não-potência-de-2, colisão improvável), e -- a UMA extensão que este arquivo
//     soma -- quando ARMADO e aquele tamanho exato é pedido, LANÇAR `std::bad_alloc` imediatamente
//     em vez de delegar pro `std::malloc`, e então DESARMAR imediatamente (disparo único: só a UMA
//     alocação casante que este arquivo mira é algum dia atingida, nada mais no mesmo processo).
//     Isto não é um mock da função sob teste: a exceção é um `std::bad_alloc` REAL, lançado do
//     alocador global REAL, no sítio de chamada REAL (o próprio `resize()` de
//     `decode_premultiplied_rgba()` pro Grupo L, a própria range-construção de
//     `std::vector<unsigned char> premultiplied(...)` do `create_texture()` pro Grupo C) --
//     exatamente o que uma falha genuína de alocador parece do próprio ponto de vista do código
//     chamador, sem nenhum do raio de explosão de driver-GL que exaustão de memória real carrega
//     num processo deste tamanho. `target_size` é computado ANALITICAMENTE (`w * h * 4`, a
//     contagem exata de bytes que as duas alocações sob teste pedem, batendo com o próprio
//     alocador `std::vector<unsigned char>` do libstdc++, que pede `n * sizeof(unsigned char)`
//     bytes sem arredondamento próprio) -- nenhum filho de calibração, nenhuma medição, nenhuma
//     margem, nenhuma variância execução-a-execução: a falha injetada dispara em TODA execução, de
//     forma determinística, é armada só imediatamente em volta da ÚNICA chamada sob teste (uma
//     chamada "de sanidade" DESARMADA do MESMO `call_under_test` roda primeiro e precisa ter
//     sucesso normal, provando que a própria fixture é válida antes da chamada armada provar o
//     conserto), e nunca toca GL/Mesa de jeito nenhum -- a falha dispara e é capturada estritamente
//     ANTES de qualquer um dos dois métodos sob teste sequer alcançar a própria chamada
//     `glTexImage2D`, por construção (as duas alocações acontecem mais cedo no corpo da função que
//     o upload GL -- o próprio comentário de topo TEX-NOTHROW de `draw2d.cpp` em cada função).
//
//     `fork()` ainda é usado (um filho por grupo), mas SÓ pra isolamento-de-crash e relato limpo --
//     uma exceção não-capturada dentro do MESMO processo do próprio `main()` deste teste abortaria
//     o binário INTEIRO antes que os `check()`s de qualquer grupo posterior pudessem rodar; um
//     filho forkado que sofre SIGABRT em vez disso relata de forma limpa via o próprio
//     `WIFSIGNALED()` do `waitpid()`, exatamente como todo outro oráculo baseado em fork desta
//     suíte.
//
//     Dois grupos independentes, um por método `noexcept`, cada um com o PRÓPRIO tamanho-alvo
//     distintivo pra que um casamento perdido num filho de um grupo nunca possa ser confundido com
//     o do outro (embora cada um rode no PRÓPRIO filho forkado de qualquer forma, então colisão
//     entre grupos nunca foi de fato possível -- mantidos distintos assim mesmo, por um rastro de
//     DEBUG sem ambiguidade se algum dos dois grupos algum dia precisar de um):
//       Grupo L (`load_texture()`) -- `137x101` px (`55348` bytes), as MESMAS dimensões
//       distintivas que o próprio Grupo A de `image_decode_hardening_sanity.cpp` já usa e provou
//       livre de colisão NESTE mesmo codebase (embora aquela prova fosse num processo livre de GL;
//       uma execução fresca, com contexto GL, aqui reconfirma isso). A fixture PNG é construída via
//       o próprio `encode_image_memory()` PÚBLICO desta biblioteca (`glintfx/image.hpp`,
//       IMG-ENCODE/ENC-NOTHROW, W21) -- nenhum PNG/zlib/CRC32 feito à mão necessário (o Grupo L não
//       precisa do truque de fonte paletizada de que o próprio Grupo B de
//       `image_decode_hardening_sanity.cpp` depende pra manter o PRÓPRIO pico interno do stb abaixo
//       da própria cópia desta biblioteca: ESTE arquivo força a falha direto, por tamanho, não
//       esgotando um orçamento de memória derivado, então qualquer PNG válido, decodificável, da
//       contagem certa de pixel funciona).
//       Grupo C (`create_texture()`) -- `179x151` px (`108116` bytes), nenhum decode/PNG envolvido
//       de jeito nenhum: um buffer de pixel `Rgba8` simples, de posse do chamador, construído
//       direto em memória.
//
//     TESTADO POR MUTAÇÃO (disciplina da casa, `feedback_auditoria_domino`): os dois grupos foram
//     provados ficarem VERMELHOS contra um build com o `try`/`catch` removido do método sob teste
//     (o filho armado sofre SIGABRT em vez de sair com 0) antes deste arquivo ser considerado
//     pronto -- ver as próprias notas de entrega desta fatia pro procedimento exato (comita o
//     conserto primeiro, muta o arquivo rastreado transitoriamente, rebuilda, observa vermelho,
//     `git checkout --` restaura, rebuilda verde de novo).
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
#include <glintfx/image.hpp>
#include "gl_loader.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <new>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using glintfx::Draw2d;
using glintfx::Texture2d;
using glintfx::WindowGlfw;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// ===========================================================================================
// EN: Single-shot, size-matched fault injection -- see this file's own top comment for the full
//     rationale (why this replaced fork()+RLIMIT_AS here specifically). `g_armed` gates the
//     override so ONLY the deliberately-targeted call is ever affected; `g_target_size` is the
//     exact byte count to match. A match immediately disarms itself (`g_armed = false`) BEFORE
//     throwing, so a caller that somehow retries the same allocation size inside its own exception
//     handling never gets hit twice.
// PT: Injeção de falha de disparo único, casada por tamanho -- ver o próprio comentário de topo
//     deste arquivo pro racional completo (por que isto substituiu o fork()+RLIMIT_AS aqui
//     especificamente). `g_armed` restringe o override pra que SÓ a chamada deliberadamente
//     mirada seja algum dia afetada; `g_target_size` é a contagem exata de bytes a casar. Um
//     casamento se desarma imediatamente (`g_armed = false`) ANTES de lançar, pra que um chamador
//     que de alguma forma tente de novo o mesmo tamanho de alocação dentro do próprio tratamento
//     de exceção nunca seja atingido duas vezes.
// ===========================================================================================
bool g_armed = false;
std::size_t g_target_size = 0;

} // namespace

void* operator new(std::size_t sz) {
  if (g_armed && sz == g_target_size) {
    g_armed = false;
    throw std::bad_alloc();
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

// EN: One forked child per group. `setup_and_sanity` stands up the child's OWN WindowGlfw+Draw2d+
//     init() (see this file's own top comment: the PARENT never touches GL/GLFW/X11) and runs an
//     UNARMED sanity call of `call_under_test`, which MUST succeed (proves the fixture itself is
//     valid, e.g. right pixel count / right dimensions, before the armed call below is trusted to
//     mean anything). Then arms `target_size` and runs `call_under_test` a SECOND time -- the
//     injected `std::bad_alloc` must strike inside that second call, at the exact allocation this
//     group targets, and the method under test must degrade to `ok() == false` without crashing.
//     Exit codes: 4 = window create failed, 5 = Draw2d::init() failed (both SKIPPED, inconclusive),
//     6 = the UNARMED sanity call itself failed (a genuine FAIL -- the fixture is broken), 0 = the
//     armed call correctly reported ok()==false, 1 = the armed call unexpectedly reported ok()==true
//     (the injected throw never landed -- a genuine FAIL, not a skip).
// PT: Um filho forkado por grupo. `setup_and_sanity` monta o PRÓPRIO WindowGlfw+Draw2d+init() do
//     filho (ver o próprio comentário de topo deste arquivo: o PAI nunca toca GL/GLFW/X11) e roda
//     uma chamada de sanidade DESARMADA de `call_under_test`, que PRECISA ter sucesso (prova que a
//     própria fixture é válida, ex.: contagem de pixel certa / dimensões certas, antes da chamada
//     armada abaixo ser confiada a significar algo). Então arma `target_size` e roda
//     `call_under_test` uma SEGUNDA vez -- o `std::bad_alloc` injetado precisa atingir dentro
//     daquela segunda chamada, na alocação exata que este grupo mira, e o método sob teste precisa
//     degradar pra `ok() == false` sem crashar. Códigos de saída: 4 = criação de janela falhou,
//     5 = Draw2d::init() falhou (ambos PULADOS, inconclusivo), 6 = a PRÓPRIA chamada de sanidade
//     DESARMADA falhou (uma FALHA genuína -- a fixture está quebrada), 0 = a chamada armada
//     reportou corretamente ok()==false, 1 = a chamada armada reportou inesperadamente ok()==true
//     (o lançamento injetado nunca chegou -- uma FALHA genuína, não um pulo).
ChildOutcome run_injection_child(std::size_t target_size,
                                 const std::function<bool(Draw2d&)>& call_under_test) {
  ChildOutcome out;
  const pid_t pid = fork();
  if (pid < 0) return out;
  out.spawned = true;

  if (pid == 0) {
    WindowGlfw window;
    if (!window.create("draw2d_texture_nothrow_child", 64, 64)) _exit(4);
    Draw2d d2d;
    if (!d2d.init()) _exit(5);

    // EN: Sanity: unarmed, must succeed -- proves the fixture is valid before the armed call
    //     below is trusted.
    // PT: Sanidade: desarmada, precisa ter sucesso -- prova que a fixture é válida antes da
    //     chamada armada abaixo ser confiada.
    if (!call_under_test(d2d)) _exit(6);

    // EN: THE injected failure. If TEX-NOTHROW regresses (the try/catch removed, or the
    //     allocation genuinely escapes uncaught), std::bad_alloc propagates uncaught ->
    //     std::terminate() -> abort() -> this process dies by SIGABRT, and the parent's
    //     WIFSIGNALED() check below catches it.
    // PT: A falha injetada. Se o TEX-NOTHROW regredir (o try/catch removido, ou a alocação
    //     genuinamente escapar não-capturada), std::bad_alloc se propaga não-capturada ->
    //     std::terminate() -> abort() -> este processo morre por SIGABRT, e a checagem
    //     WIFSIGNALED() do pai abaixo pega isso.
    g_target_size = target_size;
    g_armed = true;
    const bool ok = call_under_test(d2d);
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
void run_injection_oracle(const char* label, std::size_t target_size,
                          const std::function<bool(Draw2d&)>& call_under_test) {
  char msg[256];

  const ChildOutcome r = run_injection_child(target_size, call_under_test);
  std::snprintf(msg, sizeof(msg), "%s: child's fork() succeeded", label);
  check(r.spawned, msg);

  std::snprintf(msg, sizeof(msg),
                "%s (TEX-NOTHROW): child was NOT killed by a signal (uncaught std::bad_alloc "
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
  // ===========================================================================================
  // Group L: Draw2d::load_texture() -- decode_premultiplied_rgba()'s own internal resize()
  // (image_decode.hpp), targeted by its exact byte count.
  // ===========================================================================================
  {
    constexpr int kW = 137;
    constexpr int kH = 101;
    constexpr std::size_t kTargetSize =
        static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4u;
    check(kTargetSize == 55348u, "Group L setup: expected decoded pixel byte count is 55348");

    std::vector<unsigned char> pixels(kTargetSize);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
      pixels[i + 0] = 120;
      pixels[i + 1] = 60;
      pixels[i + 2] = 200;
      pixels[i + 3] = 255;
    }
    const glintfx::EncodedImageBytes enc =
        glintfx::encode_image_memory(glintfx::ImageFormat::Png, kW, kH, pixels.data());
    check(enc.ok, "Group L setup: encode_image_memory() produced a real PNG");

    char tmpl[] = "/var/tmp/glintfx_draw2d_texture_nothrow_XXXXXX";
    const int fd = mkstemp(tmpl);
    bool have_file = false;
    if (fd >= 0 && enc.ok) {
      std::FILE* f = fdopen(fd, "wb");
      if (f) {
        have_file = std::fwrite(enc.bytes.data(), 1, enc.bytes.size(), f) == enc.bytes.size();
        std::fclose(f);
      } else {
        close(fd);
      }
    }
    check(have_file, "Group L setup: wrote the self-built PNG to a scratch file");

    if (have_file) {
      const std::string path(tmpl);
      run_injection_oracle("Group L (load_texture)", kTargetSize, [&path](Draw2d& d2d) {
        return d2d.load_texture(path.c_str()).ok();
      });
    }
    std::remove(tmpl);
  }

  // ===========================================================================================
  // Group C: Draw2d::create_texture() -- the Rgba8 ingest allocation (the premultiplied copy of
  // the caller-supplied buffer, draw2d.cpp), targeted by its exact byte count.
  // ===========================================================================================
  {
    constexpr int kW = 179;
    constexpr int kH = 151;
    constexpr std::size_t kTargetSize =
        static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4u;
    check(kTargetSize == 108116u, "Group C setup: expected pixel byte count is 108116");

    std::vector<unsigned char> pixels(kTargetSize);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
      pixels[i + 0] = 30;
      pixels[i + 1] = 160;
      pixels[i + 2] = 90;
      pixels[i + 3] = 200; // non-255 alpha: forces the premultiply loop to touch every byte.
    }

    run_injection_oracle("Group C (create_texture)", kTargetSize, [&pixels](Draw2d& d2d) {
      return d2d.create_texture(pixels.data(), kW, kH, Draw2d::PixelFormat::Rgba8).ok();
    });
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "draw2d_texture_nothrow_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("draw2d_texture_nothrow_sanity: PASS");
  return 0;
}
