// SPDX-License-Identifier: Apache-2.0
// EN: capture_nothrow_sanity (CAPTURE-NOTHROW, W22, 2026-07-30) -- the FIFTH member of the
//     `never a crash` family (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`,
//     `image_decode_hardening_sanity.cpp`/`image_encode_hardening_sanity.cpp`/
//     `draw2d_texture_nothrow_sanity.cpp`/`font_nothrow_sanity.cpp`), and the first found by the
//     promise-vs-code MATRIX audit (W22) rather than by a consumer bug report or a literal-phrase
//     grep: `Engine::capture_frame()` (`engine.cpp`, shared by `App::capture_frame()`/
//     `UiLayer::capture_frame()`) and `capture_framebuffer()` (`frame_capture.cpp`, the
//     instance-free sibling, CAPTURE-FREE) BOTH allocate two buffers -- an intermediate `rgb`
//     scratch (`w*h*3` bytes) and the final `pixels` output (`w*h*4` bytes), coexisting
//     simultaneously during the flip/expand loop -- with NO guard against `std::bad_alloc`
//     before this fix, despite each one's own doc-comment already promising a clean,
//     default-constructed result on every OTHER documented failure. Proves BOTH fixes with the
//     SAME size-matched `operator new` fault-injection oracle `draw2d_texture_nothrow_sanity.cpp`/
//     `font_nothrow_sanity.cpp` already established (fork() only for crash isolation, not
//     RLIMIT_AS -- see those files' own top comments for the measured reason a live GL/RmlUi
//     process's own ~2.5 GiB baseline `VmSize` makes RLIMIT_AS unreliable here).
//
//     TWO INDEPENDENT ALLOCATION SITES, TWO DISTINCT TARGET SIZES, NO OCCURRENCE COUNT NEEDED
//     (unlike `font_nothrow_sanity.cpp`'s own wrinkle, where `load_font()`'s two allocations
//     coincided in byte count): the intermediate `rgb` buffer is `w*h*3` bytes and the final
//     `pixels` buffer is `w*h*4` bytes -- for any `w`/`h` this file picks, these two counts are
//     NEVER equal (`w*h*3 == w*h*4` would require `w*h == 0`, already rejected by the `w<=0||
//     h<=0` guard before either allocation runs), so a plain single-shot size match, armed once
//     per targeted call, unambiguously hits the ONE allocation each group means to test -- no
//     `g_skip_remaining`-style occurrence counter required here.
//
//     FOUR GROUPS, two functions x two allocation sites each, each with its OWN distinctive
//     window size so a stray match in one group's forked child can never be confused with
//     another's (though cross-group collision was never actually possible anyway -- each group
//     runs in its own forked child, same reasoning `draw2d_texture_nothrow_sanity.cpp`'s own top
//     comment already states for its own two groups):
//       Group E1/E2 (`Engine::capture_frame()`, driven WHITE-BOX -- `WindowGlfw` + a standalone
//       `Engine::attach()`, the EXACT pattern `ui_layer_capture_frame_smoke.cpp`'s own Part G2
//       already established for degenerate-guard coverage of this same method, reused here for
//       fault injection instead) -- `113x97` window, `E1` targets the `rgb` scratch (`113*97*3`
//       = `32883` bytes), `E2` targets the final `pixels` buffer (`113*97*4` = `43844` bytes).
//       Covers `App::capture_frame()`/`UiLayer::capture_frame()` TRANSITIVELY (both delegate to
//       this exact method, FRAMEGRAB-EMBED) without needing either public wrapper's own
//       RmlUi-document/UI-layer setup -- the white-box `Engine` alone is the shared implementation
//       under test, same "test the shared seam once, not every façade separately" discipline this
//       suite already follows elsewhere (e.g. `image_decode_hardening_sanity.cpp` testing the
//       free-function decode pair directly rather than through `Draw2d::load_texture()`).
//       Group F1/F2 (`capture_framebuffer()`, the instance-free sibling, CAPTURE-FREE) -- `149x131`
//       window, `F1` targets the `rgb` scratch (`149*131*3` = `58557` bytes), `F2` targets the
//       final `pixels` buffer (`149*131*4` = `78076` bytes). No `Engine`/`UiLayer` needed at all
//       here -- a plain `WindowGlfw` already satisfies the loader-not-ready guard's own
//       precondition (`glx_gl_load()` runs as a side effect of `WindowGlfw::create()`, see
//       `frame_capture.hpp`'s own "PRE-REQUISITE" section), matching this function's own design
//       point: reading pixels back does not require knowing RmlUi exists.
//
//     DECLARED COVERAGE NOTE (not a gap, a design fact worth stating): `capture_framebuffer()`
//     also gained a 256 MiB byte-count cap in this same fix (CAPTURE-NOTHROW, see
//     `frame_capture.cpp`'s own top comment for the full derivation) -- this file's own `F1`/`F2`
//     target sizes (58557/78076 bytes) sit FAR below that cap on purpose, so the injected
//     `std::bad_alloc` is what triggers the degrade, not the cap rejecting the request outright
//     before either allocation is ever attempted (a cap-triggered rejection would prove the cap
//     works, a DIFFERENT and already-covered-by-inspection property, not that the `try`/`catch`
//     itself catches a REAL allocation failure at a size the cap would have let through).
// PT: capture_nothrow_sanity (CAPTURE-NOTHROW, W22, 2026-07-30) -- o QUINTO membro da família
//     `never a crash` (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`,
//     `image_decode_hardening_sanity.cpp`/`image_encode_hardening_sanity.cpp`/
//     `draw2d_texture_nothrow_sanity.cpp`/`font_nothrow_sanity.cpp`), e o primeiro achado pela
//     auditoria de MATRIZ promessa-vs-código (W22) em vez de um relato de bug de consumidor ou um
//     grep de frase literal: `Engine::capture_frame()` (`engine.cpp`, compartilhado por
//     `App::capture_frame()`/`UiLayer::capture_frame()`) e `capture_framebuffer()`
//     (`frame_capture.cpp`, a irmã sem instância, CAPTURE-FREE) alocam OS DOIS dois buffers -- um
//     scratch `rgb` intermediário (`w*h*3` bytes) e o `pixels` final de saída (`w*h*4` bytes),
//     coexistindo simultaneamente durante o laço de flip/expansão -- SEM guarda nenhuma contra
//     `std::bad_alloc` antes deste conserto, apesar de cada um já prometer no próprio doc-comment
//     um resultado limpo, default-construído, em toda OUTRA falha documentada. Prova OS DOIS
//     consertos com o MESMO oráculo de injeção de falha em `operator new`, casado por tamanho,
//     que `draw2d_texture_nothrow_sanity.cpp`/`font_nothrow_sanity.cpp` já estabeleceram (fork()
//     só por isolamento de crash, não RLIMIT_AS -- ver o próprio comentário de topo daqueles
//     arquivos pro motivo medido de o próprio `VmSize` baseline de ~2,5 GiB de um processo
//     GL/RmlUi vivo tornar o RLIMIT_AS pouco confiável aqui).
//
//     DOIS SÍTIOS DE ALOCAÇÃO INDEPENDENTES, DOIS TAMANHOS-ALVO DISTINTOS, NENHUMA CONTAGEM DE
//     OCORRÊNCIA NECESSÁRIA (diferente da própria ruga de `font_nothrow_sanity.cpp`, onde as duas
//     alocações do `load_font()` coincidiam em contagem de bytes): o buffer intermediário `rgb` é
//     `w*h*3` bytes e o buffer `pixels` final é `w*h*4` bytes -- pra qualquer `w`/`h` que este
//     arquivo escolha, essas duas contagens NUNCA são iguais (`w*h*3 == w*h*4` exigiria `w*h ==
//     0`, já rejeitado pela guarda `w<=0||h<=0` antes de qualquer uma das duas alocações rodar),
//     então um casamento por tamanho de disparo único simples, armado uma vez por chamada mirada,
//     atinge sem ambiguidade a ÚNICA alocação que cada grupo pretende testar -- nenhum contador
//     de ocorrência estilo `g_skip_remaining` necessário aqui.
//
//     QUATRO GRUPOS, duas funções x dois sítios de alocação cada, cada um com o PRÓPRIO tamanho
//     de janela distintivo pra que um casamento perdido no filho forkado de um grupo nunca possa
//     ser confundido com o de outro (embora colisão entre grupos nunca tenha sido possível de
//     qualquer forma -- cada grupo roda no PRÓPRIO filho forkado, mesmo racional que o próprio
//     comentário de topo de `draw2d_texture_nothrow_sanity.cpp` já afirma pros próprios dois
//     grupos dele):
//       Grupo E1/E2 (`Engine::capture_frame()`, dirigido WHITE-BOX -- `WindowGlfw` + um
//       `Engine::attach()` avulso, o EXATO padrão que a própria Parte G2 de
//       `ui_layer_capture_frame_smoke.cpp` já estabeleceu pra cobertura de guarda-degenerada
//       deste mesmo método, reusado aqui pra injeção de falha em vez disso) -- janela `113x97`,
//       `E1` mira o scratch `rgb` (`113*97*3` = `32883` bytes), `E2` mira o buffer `pixels` final
//       (`113*97*4` = `43844` bytes). Cobre `App::capture_frame()`/`UiLayer::capture_frame()`
//       TRANSITIVAMENTE (os dois delegam pra este exato método, FRAMEGRAB-EMBED) sem precisar do
//       próprio setup de documento-RmlUi/camada-de-UI de nenhum dos dois wrappers públicos -- o
//       `Engine` white-box sozinho é a implementação compartilhada sob teste, mesma disciplina
//       "testa a costura compartilhada uma vez, não cada fachada separadamente" que esta suíte já
//       segue em outro lugar (ex.: `image_decode_hardening_sanity.cpp` testando o par de decode
//       de free-function direto em vez de através do `Draw2d::load_texture()`).
//       Grupo F1/F2 (`capture_framebuffer()`, a irmã sem instância, CAPTURE-FREE) -- janela
//       `149x131`, `F1` mira o scratch `rgb` (`149*131*3` = `58557` bytes), `F2` mira o buffer
//       `pixels` final (`149*131*4` = `78076` bytes). Nenhum `Engine`/`UiLayer` necessário aqui --
//       um `WindowGlfw` puro já satisfaz a própria precondição da guarda de loader-não-pronto
//       (`glx_gl_load()` roda como efeito colateral do `WindowGlfw::create()`, ver a própria seção
//       "PRÉ-REQUISITO" de `frame_capture.hpp`), batendo com o próprio ponto de desenho desta
//       função: ler pixels de volta não exige saber que a RmlUi existe.
//
//     NOTA DE COBERTURA DECLARADA (não uma lacuna, um fato de desenho que vale dizer): o
//     `capture_framebuffer()` também ganhou um teto de 256 MiB de contagem de bytes neste mesmo
//     conserto (CAPTURE-NOTHROW, ver o próprio comentário de topo de `frame_capture.cpp` pra
//     derivação completa) -- os próprios tamanhos-alvo `F1`/`F2` deste arquivo (58557/78076
//     bytes) ficam BEM abaixo daquele teto de propósito, então o `std::bad_alloc` injetado é o
//     que dispara a degradação, não o teto rejeitando o pedido de saída antes de qualquer uma das
//     duas alocações sequer ser tentada (uma rejeição disparada-pelo-teto provaria que o teto
//     funciona, uma propriedade DIFERENTE e já coberta por inspeção, não que o próprio
//     `try`/`catch` captura uma falha de alocação REAL num tamanho que o teto deixaria passar).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>

#include <sys/wait.h>
#include <unistd.h>

using glintfx::CapturedFramebuffer;
using glintfx::CapturedFramePixels;
using glintfx::Engine;
using glintfx::SystemClock;
using glintfx::WindowGlfw;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Single-shot, size-matched fault injection -- see this file's own top comment for the full
//     rationale, and draw2d_texture_nothrow_sanity.cpp's own top comment for the technique's
//     original derivation. No occurrence counter needed here (see this file's own top comment):
//     `g_armed` gates the override so ONLY the deliberately-targeted allocation is ever affected;
//     a match immediately disarms itself (`g_armed = false`) BEFORE throwing.
// PT: Injeção de falha de disparo único, casada por tamanho -- ver o próprio comentário de topo
//     deste arquivo pro racional completo, e o próprio comentário de topo de
//     draw2d_texture_nothrow_sanity.cpp pra derivação original da técnica. Nenhum contador de
//     ocorrência necessário aqui (ver o próprio comentário de topo deste arquivo): `g_armed`
//     restringe o override pra que SÓ a alocação deliberadamente mirada seja algum dia afetada;
//     um casamento se desarma imediatamente (`g_armed = false`) ANTES de lançar.
bool g_armed = false;
std::size_t g_target_size = 0;

// EN: ARRAY-NEW-COUNTED (bug found post-commit by the team lead, 2026-07-30): `rgb`
//     (`std::vector<unsigned char>`, E1/F1) allocates via the SCALAR `operator new(size_t)`
//     below -- but `pixels` (`std::make_unique<unsigned char[]>`, E2/F2, `frame_capture.cpp`/
//     `engine.cpp`'s own `make_unique<unsigned char[]>(byte_count)`) allocates via a
//     COMPLETELY SEPARATE overload, `operator new[](size_t)`, which this file did NOT override
//     at first. In a NORMAL (non-sanitized) libstdc++ build, the default `operator new[]`
//     happens to forward to the scalar `operator new` this file DOES override -- so E2/F2
//     "worked", but by an implementation-defined forwarding accident, not because the injected
//     fault ever reached the array-new path this file's own doc-comment claims to target. Under
//     `GLINTFX_SANITIZE=ON` ASan installs its OWN array-new that does NOT forward, so the
//     injection silently misses and E2/F2 would fail on the heavy/ASan CI leg -- this is
//     precisely the "right result, accidental mechanism" class this whole `never a crash`
//     family exists to rule out, discovered inside the very test built to catch it. `g_new_
//     array_calls` below is unconditional (increments on EVERY array-new call, armed or not,
//     matching or not) SPECIFICALLY so E2/F2 can prove-by-execution that THIS override, not
//     some other code path, is what ran for the `pixels` allocation -- see the counter
//     snapshot-and-check around each group's own UNARMED sanity call below.
// PT: ARRAY-NEW-CONTADO (bug achado depois do commit pelo team lead, 2026-07-30): o `rgb`
//     (`std::vector<unsigned char>`, E1/F1) aloca via o `operator new(size_t)` ESCALAR abaixo --
//     mas o `pixels` (`std::make_unique<unsigned char[]>`, E2/F2, o próprio
//     `make_unique<unsigned char[]>(byte_count)` de `frame_capture.cpp`/`engine.cpp`) aloca via
//     uma sobrecarga COMPLETAMENTE SEPARADA, `operator new[](size_t)`, que este arquivo NÃO
//     sobrescrevia a princípio. Num build NORMAL (sem sanitizer) da libstdc++, o `operator
//     new[]` default por acaso encaminha pro `operator new` escalar que este arquivo JÁ
//     sobrescreve -- então E2/F2 "funcionavam", mas por um acidente de encaminhamento definido
//     pela implementação, não porque a falha injetada de fato alcançasse o caminho de array-new
//     que o próprio comentário de topo deste arquivo afirma mirar. Sob `GLINTFX_SANITIZE=ON` o
//     ASan instala o PRÓPRIO array-new que NÃO encaminha, então a injeção erra em silêncio e
//     E2/F2 falhariam na perna heavy/ASan do CI -- exatamente a classe "resultado certo,
//     mecanismo acidental" que esta família `never a crash` inteira existe pra descartar,
//     descoberta dentro do próprio teste construído pra caçá-la. O `g_new_array_calls` abaixo é
//     incondicional (incrementa em TODA chamada de array-new, armada ou não, casando ou não) DE
//     PROPÓSITO pra que E2/F2 provem por execução que ESTE override, não algum outro caminho de
//     código, é quem rodou pra alocação de `pixels` -- ver o snapshot-e-checagem do contador ao
//     redor da própria chamada de sanidade DESARMADA de cada grupo abaixo.
std::size_t g_new_array_calls = 0;

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

void* operator new[](std::size_t sz) {
  ++g_new_array_calls;
  if (g_armed && sz == g_target_size) {
    g_armed = false;
    throw std::bad_alloc();
  }
  void* p = std::malloc(sz);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}

void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {

struct ChildOutcome {
  bool spawned = false;
  bool signalled = false;
  int term_signal = 0;
  bool exited = false;
  int exit_code = -1;
};

// EN: One forked child per group. `call_under_test` returns whether the call reported `ok()`
//     true. Exit codes mirror draw2d_texture_nothrow_sanity.cpp's own convention: 4 = window
//     create failed, 5 = Engine::attach() failed (Group E1/E2 only -- both SKIPPED,
//     inconclusive), 6 = the UNARMED sanity call itself failed (a genuine FAIL -- the fixture is
//     broken), 0 = the armed call correctly reported ok()==false, 1 = the armed call
//     unexpectedly reported ok()==true (the injected throw never landed -- a genuine FAIL, not a
//     skip), 7 = ARRAY-NEW-COUNTED (Group E2/F2 only): `g_new_array_calls` did not increase
//     across the UNARMED sanity call -- `operator new[]` was never observed to fire for the
//     `pixels` allocation, meaning this run cannot prove the injected fault reaches the array-new
//     path at all (a genuine FAIL: the size-matched arming below would be meaningless without
//     this).
// PT: Um filho forkado por grupo. `call_under_test` retorna se a chamada reportou `ok()` true.
//     Códigos de saída espelham a própria convenção de draw2d_texture_nothrow_sanity.cpp:
//     4 = criação de janela falhou, 5 = Engine::attach() falhou (só Grupo E1/E2 -- ambos
//     PULADOS, inconclusivo), 6 = a PRÓPRIA chamada de sanidade DESARMADA falhou (uma FALHA
//     genuína -- a fixture está quebrada), 0 = a chamada armada reportou corretamente
//     ok()==false, 1 = a chamada armada reportou inesperadamente ok()==true (o lançamento
//     injetado nunca chegou -- uma FALHA genuína, não um pulo), 7 = ARRAY-NEW-CONTADO (só Grupo
//     E2/F2): `g_new_array_calls` não aumentou através da chamada de sanidade DESARMADA -- o
//     `operator new[]` nunca foi observado disparando pra alocação de `pixels`, o que significa
//     que esta rodada não consegue provar que a falha injetada sequer alcança o caminho de
//     array-new (uma FALHA genuína: o armamento casado-por-tamanho abaixo seria sem sentido sem
//     isto).
ChildOutcome run_injection_child(std::size_t target_size,
                                 const std::function<int()>& setup_and_call) {
  ChildOutcome out;
  const pid_t pid = fork();
  if (pid < 0) return out;
  out.spawned = true;

  if (pid == 0) {
    // EN: setup_and_call() does its own WindowGlfw/Engine setup, an UNARMED sanity call first,
    //     then arms target_size and calls again -- mirrors
    //     draw2d_texture_nothrow_sanity.cpp's own run_injection_child() shape, adapted so each
    //     group can do its own distinct setup (Engine::attach() for E1/E2, none for F1/F2)
    //     without duplicating fork()/waitpid() four times.
    // PT: setup_and_call() faz o próprio setup WindowGlfw/Engine, uma chamada de sanidade
    //     DESARMADA primeiro, depois arma target_size e chama de novo -- espelha a própria forma
    //     de run_injection_child() de draw2d_texture_nothrow_sanity.cpp, adaptada pra que cada
    //     grupo faça o PRÓPRIO setup distinto (Engine::attach() pra E1/E2, nenhum pra F1/F2) sem
    //     duplicar fork()/waitpid() quatro vezes.
    g_target_size = target_size;
    const int code = setup_and_call();
    _exit(code);
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
// PT: Aplica a classificação PASSA/PULA/FALHA descrita no próprio doc-comment de
//     run_injection_child().
void run_injection_oracle(const char* label, std::size_t target_size,
                          const std::function<int()>& setup_and_call) {
  char msg[256];

  const ChildOutcome r = run_injection_child(target_size, setup_and_call);
  std::snprintf(msg, sizeof(msg), "%s: child's fork() succeeded", label);
  check(r.spawned, msg);

  std::snprintf(msg, sizeof(msg),
                "%s (CAPTURE-NOTHROW): child was NOT killed by a signal (uncaught std::bad_alloc "
                "would SIGABRT) -- WIFSIGNALED=%d, signal=%d",
                label, r.signalled ? 1 : 0, r.signalled ? r.term_signal : 0);
  check(!r.signalled, msg);

  if (r.signalled || !r.exited) return;

  if (r.exit_code == 4) {
    std::fprintf(stderr, "%s: SKIPPED -- child's window create failed -- inconclusive\n", label);
  } else if (r.exit_code == 5) {
    std::fprintf(stderr, "%s: SKIPPED -- child's Engine::attach() failed -- inconclusive\n", label);
  } else if (r.exit_code == 6) {
    std::snprintf(msg, sizeof(msg),
                  "%s setup: the UNARMED sanity call succeeded (fixture itself is valid)", label);
    check(false, msg);
  } else if (r.exit_code == 7) {
    std::snprintf(msg, sizeof(msg),
                  "%s setup: g_new_array_calls increased across the UNARMED sanity call -- "
                  "operator new[] confirmed as the mechanism for the pixels allocation, not a "
                  "scalar-forwarding accident",
                  label);
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
  // Group E1/E2: Engine::capture_frame() -- white-box, WindowGlfw + a standalone Engine::attach()
  // (same pattern ui_layer_capture_frame_smoke.cpp's own Part G2 already established), covering
  // App::capture_frame()/UiLayer::capture_frame() transitively (both delegate to this method).
  // ===========================================================================================
  {
    constexpr int kW = 113;
    constexpr int kH = 97;
    constexpr std::size_t kRgbBytes = static_cast<std::size_t>(kW) * kH * 3u;
    constexpr std::size_t kPixelsBytes = static_cast<std::size_t>(kW) * kH * 4u;
    check(kRgbBytes == 32883u, "Group E setup: expected rgb scratch byte count is 32883");
    check(kPixelsBytes == 43844u, "Group E setup: expected pixels byte count is 43844");

    run_injection_oracle("Group E1 (Engine::capture_frame rgb scratch)", kRgbBytes,
                         []() -> int {
                           WindowGlfw window;
                           if (!window.create("capture_nothrow_e1_child", kW, kH)) return 4;
                           SystemClock clock;
                           Engine engine;
                           if (!engine.attach(&clock, kW, kH)) return 5;
                           if (!engine.capture_frame(0, 0, kW, kH).ok) return 6;
                           g_armed = true;
                           const bool ok = engine.capture_frame(0, 0, kW, kH).ok;
                           g_armed = false;
                           return ok ? 1 : 0;
                         });

    run_injection_oracle("Group E2 (Engine::capture_frame pixels buffer)", kPixelsBytes,
                         []() -> int {
                           WindowGlfw window;
                           if (!window.create("capture_nothrow_e2_child", kW, kH)) return 4;
                           SystemClock clock;
                           Engine engine;
                           if (!engine.attach(&clock, kW, kH)) return 5;
                           const std::size_t before = g_new_array_calls;
                           if (!engine.capture_frame(0, 0, kW, kH).ok) return 6;
                           if (g_new_array_calls == before) return 7; // ARRAY-NEW-COUNTED.
                           g_armed = true;
                           const bool ok = engine.capture_frame(0, 0, kW, kH).ok;
                           g_armed = false;
                           return ok ? 1 : 0;
                         });
  }

  // ===========================================================================================
  // Group F1/F2: capture_framebuffer() -- instance-free, a plain WindowGlfw already satisfies
  // the loader-not-ready guard's own precondition.
  // ===========================================================================================
  {
    constexpr int kW = 149;
    constexpr int kH = 131;
    constexpr std::size_t kRgbBytes = static_cast<std::size_t>(kW) * kH * 3u;
    constexpr std::size_t kPixelsBytes = static_cast<std::size_t>(kW) * kH * 4u;
    check(kRgbBytes == 58557u, "Group F setup: expected rgb scratch byte count is 58557");
    check(kPixelsBytes == 78076u, "Group F setup: expected pixels byte count is 78076");

    run_injection_oracle("Group F1 (capture_framebuffer rgb scratch)", kRgbBytes,
                         []() -> int {
                           WindowGlfw window;
                           if (!window.create("capture_nothrow_f1_child", kW, kH)) return 4;
                           if (!glintfx::capture_framebuffer(0, 0, kW, kH).ok) return 6;
                           g_armed = true;
                           const bool ok = glintfx::capture_framebuffer(0, 0, kW, kH).ok;
                           g_armed = false;
                           return ok ? 1 : 0;
                         });

    run_injection_oracle("Group F2 (capture_framebuffer pixels buffer)", kPixelsBytes,
                         []() -> int {
                           WindowGlfw window;
                           if (!window.create("capture_nothrow_f2_child", kW, kH)) return 4;
                           const std::size_t before = g_new_array_calls;
                           if (!glintfx::capture_framebuffer(0, 0, kW, kH).ok) return 6;
                           if (g_new_array_calls == before) return 7; // ARRAY-NEW-COUNTED.
                           g_armed = true;
                           const bool ok = glintfx::capture_framebuffer(0, 0, kW, kH).ok;
                           g_armed = false;
                           return ok ? 1 : 0;
                         });
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "capture_nothrow_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("capture_nothrow_sanity: PASS");
  return 0;
}
