// SPDX-License-Identifier: MPL-2.0
// EN: app_window_icon_sanity -- contract test for `WIN-ICON` (framework-2D, consumer-driven --
//     GusWorld decodes its own PNG icon with stb_image and calls SDL_SetWindowIcon today;
//     App::set_window_icon closes the gap that would otherwise lose the window icon on cutover
//     to App).
//
//     HEADLESS HONESTY (declared, not faked -- same policy `app_vsync_sanity.cpp` states):
//     under Xvfb there is no window manager, so there is no taskbar/titlebar to visually
//     confirm an icon on. This file therefore proves the CONTRACT (guards never crash, the
//     accepted path returns `true` and the underlying `glfwSetWindowIcon` call does not abort
//     the process), never that any PIXEL actually reaches a visible icon anywhere -- that is a
//     manual/nested-compositor QA leg, not something this suite can prove, and is named here
//     rather than silently skipped.
//
//     FORWARD-COVERAGE gate (same shape as `app_vsync_sanity.cpp`'s rationale):
//     `App::set_window_icon` is a thin forward to `WindowGlfw::set_window_icon` (src/app.cpp) --
//     a broken forward (e.g. short-circuited to always return `true` without ever reaching
//     WindowGlfw's own guards) would be invisible to a WindowGlfw-only test. [1]/[2] below
//     exercise WindowGlfw DIRECTLY (same `#include "../src/window_glfw.hpp"` pattern as
//     window_smoke.cpp/app_vsync_sanity.cpp) to pin the real guard behaviour; [3] exercises the
//     SAME accepted path through `App` specifically.
//
//     [1] PRE-CREATE GUARD (WindowGlfw, `win_ == nullptr`) -- `set_window_icon` returns `false`
//         even with an otherwise-valid pixel buffer. No crash (D5 fail-high, same shape as every
//         other WindowGlfw guard in this class).
//     [2] POST-CREATE CONTRACT (WindowGlfw, real window) -- null `pixels_rgba8` rejected
//         (`false`); non-positive `w`/`h` rejected (`false`); `w`/`h` exceeding the 1024px cap
//         (LOWERED from 2048, see [2b] below) rejected (`false`, and the caller's buffer for
//         that check is deliberately never even allocated to that oversized size -- see the
//         guard-order note on the declaration); a well-formed small RGBA8 buffer (2x2) is
//         accepted (`true`), and the caller's own buffer is provably untouched afterwards (read
//         back and compared byte-for-byte) -- confirms this call copies rather than
//         mutating/aliasing the caller's memory.
//     [3] APP FORWARD-COVERAGE -- the SAME accepted path, reached through `App::set_window_icon`;
//         ends with one poll/update/render cycle to confirm the pipeline stays live after the
//         call (same "does not break anything else" proof `app_vsync_sanity.cpp`/
//         `app_dp_ratio_smoke.cpp` use).
//     [2b] TST-ICON-BOUNDARY -- the actual cap-enforcement boundary, added because the
//         pre-existing `4096x4096` check above (kept for its own value as a well-beyond-the-cap
//         sanity check) does NOT distinguish one cap value from another: an adversarial review
//         planted the mutation `kMaxIconDim 2048 -> 2049` (back when the cap was still 2048) and
//         the official suite stayed GREEN. A standalone probe written by the reviewer then
//         caught that exact mutation -- but only because it used REAL buffers sized to the
//         DECLARED dimensions; the `4096x4096` check above reuses the tiny 16-byte `kIcon2x2`
//         buffer with an oversized declared dimension, so on that same off-by-one-cap mutation
//         it would "fail" by SIGSEGV (an accidental over-read past a 16-byte buffer), not by a
//         clean assertion -- a crash-caught mutation is luck, not a passing test.
//
//         THIS BOUNDARY TEST ITSELF FOUND A REAL BUG, not just a test gap: writing it with a
//         REAL 2048x2048 buffer (the cap's value at the time) proved `WindowGlfw::set_window_icon`
//         was NEVER actually safe to use at that cap -- `glfwSetWindowIcon`'s underlying
//         `XChangeProperty` call OVERFLOWS the X11 protocol's own max-request-size for a
//         2048x2048 icon and CRASHES the client with a fatal `BadLength` error under a real X11
//         backend. The `4096x4096` check above could never have found this: it rejects before
//         ever reaching `glfwSetWindowIcon` (its buffer is far too small to survive an ACCEPT).
//         `kMaxIconDim` was LOWERED to 1024 as a direct result (see window_glfw.cpp's own
//         doc-comment on `kMaxIconDim` for the full byte-for-byte derivation and the note on why
//         this was invisible on a live Wayland desktop session). The pair below tests the CURRENT
//         boundary (1024 accepted / 1025 rejected), both with a REAL buffer allocated to `w*h*4`
//         bytes via `make_icon_buffer()`, so a relaxed cap does not crash on the rejected case --
//         it returns `true` where the assertion expects `false`, a clean boolean failure.
// PT: [2b] TST-ICON-BOUNDARY -- a fronteira real de aplicação do teto, adicionada porque o
//         cheque pré-existente `4096x4096` acima (mantido pelo próprio valor como sanity-check
//         bem-além-do-teto) NÃO distingue um valor de teto de outro: um review adversarial
//         plantou a mutação `kMaxIconDim 2048 -> 2049` (quando o teto ainda era 2048) e a suíte
//         oficial ficou VERDE. Uma sonda avulsa escrita pelo reviewer então pegou exatamente essa
//         mutação -- mas só porque usava buffers REAIS do tamanho DECLARADO; o cheque
//         `4096x4096` acima reusa o buffer minúsculo de 16 bytes `kIcon2x2` com uma dimensão
//         declarada descomunal, então nessa mesma mutação de teto-fora-por-um ele "falharia" por
//         SIGSEGV (um over-read acidental além de um buffer de 16 bytes), não por uma asserção
//         limpa -- mutação pega por crash é sorte, não teste passando.
//
//         ESTE PRÓPRIO TESTE DE FRONTEIRA ACHOU UM BUG REAL, não só um gap de teste: escrevê-lo
//         com um buffer REAL 2048x2048 (o valor do teto na época) provou que
//         `WindowGlfw::set_window_icon` NUNCA foi seguro de usar de verdade naquele teto -- a
//         chamada `XChangeProperty` subjacente ao `glfwSetWindowIcon` ULTRAPASSA o próprio
//         max-request-size do protocolo X11 pra um ícone 2048x2048 e CRASHA o cliente com um
//         erro fatal `BadLength` sob um backend X11 real. O cheque `4096x4096` acima NUNCA
//         poderia ter achado isso: ele rejeita antes de sequer alcançar `glfwSetWindowIcon` (seu
//         buffer é pequeno demais pra sobreviver a um ACEITO). O `kMaxIconDim` foi BAIXADO pra
//         1024 como resultado direto (ver o próprio doc-comment de `kMaxIconDim` em
//         window_glfw.cpp pra derivação byte-a-byte completa e a nota de por que isso ficou
//         invisível numa sessão de desktop Wayland ao vivo). O par abaixo testa a fronteira
//         ATUAL (1024 aceito / 1025 rejeitado), os dois com um buffer REAL alocado pra `w*h*4`
//         bytes via `make_icon_buffer()`, então um teto relaxado não crasha no caso rejeitado --
//         ele retorna `true` onde a asserção espera `false`, uma falha booleana limpa.
//
// PT: app_window_icon_sanity -- teste de contrato para `WIN-ICON` (framework-2D,
//     consumer-driven -- o GusWorld decodifica o próprio PNG de ícone com stb_image e chama
//     SDL_SetWindowIcon hoje; App::set_window_icon fecha a lacuna que faria perder o ícone da
//     janela no cutover pro App).
//
//     HONESTIDADE HEADLESS (declarada, não forjada -- mesma política que
//     `app_vsync_sanity.cpp` declara): sob Xvfb não há window manager, então não há barra de
//     tarefas/titlebar pra confirmar visualmente um ícone. Este arquivo portanto prova o
//     CONTRATO (guardas nunca crasham, o caminho aceito retorna `true` e a chamada
//     `glfwSetWindowIcon` subjacente não aborta o processo), nunca que algum PIXEL de fato
//     chega a um ícone visível em algum lugar -- isso é uma perna de QA manual/compositor
//     aninhado, não algo que esta suíte consiga provar, e é nomeado aqui em vez de
//     silenciosamente pulado.
//
//     Gate de COBERTURA DE FORWARD (mesmo formato do racional de `app_vsync_sanity.cpp`):
//     `App::set_window_icon` é um repasse fino a `WindowGlfw::set_window_icon` (src/app.cpp) --
//     um forward quebrado (ex.: curto-circuitado pra sempre retornar `true` sem nunca alcançar
//     as próprias guardas do WindowGlfw) seria invisível a um teste só-WindowGlfw. [1]/[2]
//     abaixo exercitam o WindowGlfw DIRETAMENTE (mesmo padrão `#include "../src/window_glfw.hpp"`
//     de window_smoke.cpp/app_vsync_sanity.cpp) pra fixar o comportamento real da guarda; [3]
//     exercita o MESMO caminho aceito através do `App` especificamente.
//
//     [1] GUARDA PRÉ-CREATE (WindowGlfw, `win_ == nullptr`) -- `set_window_icon` retorna `false`
//         mesmo com um buffer de pixel de resto válido. Sem crash (fail-high D5, mesma forma de
//         toda outra guarda desta classe).
//     [2] CONTRATO PÓS-CREATE (WindowGlfw, janela real) -- `pixels_rgba8` nulo rejeitado
//         (`false`); `w`/`h` não-positivo rejeitado (`false`); `w`/`h` excedendo o teto de
//         1024px (BAIXADO de 2048, ver [2b] abaixo) rejeitado (`false`, e o buffer do chamador
//         pra este cheque deliberadamente nem chega a ser alocado naquele tamanho descomunal --
//         ver a nota de ordem de guarda na declaração); um buffer RGBA8 bem-formado pequeno
//         (2x2) é aceito (`true`), e o próprio buffer do chamador fica provadamente intocado
//         depois (lido de volta e comparado byte-a-byte) -- confirma que esta chamada copia em
//         vez de mutar/aliasar a memória do chamador.
//     [3] COBERTURA DE FORWARD DO APP -- o MESMO caminho aceito, alcançado através de
//         `App::set_window_icon`; termina com um ciclo poll/update/render pra confirmar que o
//         pipeline continua ativo após a chamada (mesma prova de "não quebra mais nada" que
//         `app_vsync_sanity.cpp`/`app_dp_ratio_smoke.cpp` usam).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
// EN: TST-ICON-BOUNDARY -- builds a REAL RGBA8 buffer sized exactly `w*h*4` bytes (the same
//     byte count `set_window_icon`'s own `w * h * 4` copy would use), filled with a
//     non-zero, non-uniform-looking pattern (not that the guard cares about content, but a
//     debugger dumping the buffer on a future failure sees plausible pixel-shaped data, not a
//     wall of zero bytes). This is the buffer shape the reviewer's own standalone probe used --
//     see this file's `[2b]` doc-comment above for why a small buffer with an oversized
//     DECLARED dimension is not an equivalent test.
// PT: TST-ICON-BOUNDARY -- monta um buffer RGBA8 REAL do tamanho exato `w*h*4` bytes (a mesma
//     contagem de bytes que a própria cópia `w * h * 4` de `set_window_icon` usaria), preenchido
//     com um padrão não-zero e não-uniforme (não que a guarda se importe com o conteúdo, mas um
//     debugger despejando o buffer numa falha futura vê dado com cara de pixel plausível, não
//     uma parede de bytes zero). Esta é a forma de buffer que a própria sonda avulsa do
//     reviewer usou -- ver o doc-comment `[2b]` deste arquivo acima pro porquê de um buffer
//     pequeno com dimensão DECLARADA descomunal não ser um teste equivalente.
std::vector<unsigned char> make_icon_buffer(int w, int h) {
  const std::size_t byte_count = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
  std::vector<unsigned char> buf(byte_count);
  for (std::size_t i = 0; i < byte_count; ++i) {
    buf[i] = static_cast<unsigned char>((i * 37u + 11u) & 0xFFu);
  }
  return buf;
}
} // namespace

namespace {
// EN: A tiny, well-formed 2x2 RGBA8 icon -- straight (non-premultiplied) alpha, red-first byte
//     order, as WIN-ICON's own contract requires.
// PT: Um ícone RGBA8 2x2 bem-formado e pequeno -- alpha STRAIGHT (não-premultiplicado), ordem
//     de byte vermelho-primeiro, conforme o próprio contrato do WIN-ICON exige.
constexpr unsigned char kIcon2x2[2 * 2 * 4] = {
    255,
    0,
    0,
    255,
    0,
    255,
    0,
    255,
    0,
    0,
    255,
    255,
    255,
    255,
    255,
    128,
};
} // namespace

int main() {
  int failures = 0;

  // ---------------------------------------------------------------------------
  // [1] Pre-create guard -- a default-constructed WindowGlfw never had create() called.
  // ---------------------------------------------------------------------------
  {
    glintfx::WindowGlfw w;
    if (w.set_window_icon(kIcon2x2, 2, 2)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [1] set_window_icon(...) returned true "
                   "before create()\n");
      ++failures;
    }
  }

  // ---------------------------------------------------------------------------
  // [2] Post-create contract, exercised directly on WindowGlfw (scoped so its dtor runs before
  //     the App in [3] below calls glfwInit() again -- same isolation app_vsync_sanity.cpp uses).
  // ---------------------------------------------------------------------------
  {
    glintfx::WindowGlfw w;
    if (!w.create("app_window_icon_sanity", 320, 240)) {
      std::puts("SKIP: WindowGlfw::create() failed -- no display / GL context");
      return failures;
    }

    if (w.set_window_icon(nullptr, 2, 2)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [2] set_window_icon(nullptr, ...) "
                   "returned true\n");
      ++failures;
    }
    if (w.set_window_icon(kIcon2x2, 0, 2)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [2] set_window_icon(..., w=0, ...) "
                   "returned true\n");
      ++failures;
    }
    if (w.set_window_icon(kIcon2x2, 2, -1)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [2] set_window_icon(..., h=-1) "
                   "returned true\n");
      ++failures;
    }
    if (w.set_window_icon(kIcon2x2, 4096, 4096)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [2] set_window_icon(..., 4096x4096) "
                   "returned true -- cap not enforced\n");
      ++failures;
    }

    // ---------------------------------------------------------------------------
    // [2b] TST-ICON-BOUNDARY -- the actual cap boundary, 1024 (accept) vs 1025 (reject), each
    //      with a REAL buffer sized to its declared dimensions. See this file's [2b] doc-comment
    //      above for why this pair -- not the 4096x4096 exaggeration above -- is the real
    //      regression gate for `kMaxIconDim`, and for the story of how writing this exact pair
    //      at the PREVIOUS cap (2048) found a real client-crashing bug rather than a test gap.
    // ---------------------------------------------------------------------------
    {
      const auto boundary_ok = make_icon_buffer(1024, 1024); // 1024*1024*4 == 4 MiB
      if (!w.set_window_icon(boundary_ok.data(), 1024, 1024)) {
        std::fprintf(stderr,
                     "app_window_icon_sanity FAIL: [2b] set_window_icon(..., 1024x1024) "
                     "returned false -- the cap must ACCEPT exactly at 1024\n");
        ++failures;
      }
    }
    {
      const auto boundary_reject = make_icon_buffer(1025, 1025); // 1025*1025*4 ~= 4.01 MiB
      if (w.set_window_icon(boundary_reject.data(), 1025, 1025)) {
        std::fprintf(stderr,
                     "app_window_icon_sanity FAIL: [2b] set_window_icon(..., 1025x1025) "
                     "returned true -- the cap must REJECT one px past 1024\n");
        ++failures;
      }
    }

    // EN: Caller-owned buffer, checked byte-for-byte after the call to confirm
    //     set_window_icon() copies instead of aliasing/mutating it (the "safe to
    //     free/reuse the caller's own buffer" contract on the declaration).
    // PT: Buffer de posse do chamador, checado byte-a-byte após a chamada pra confirmar que
    //     set_window_icon() copia em vez de aliasar/mutar ele (o contrato "seguro liberar/
    //     reusar o buffer do próprio chamador" da declaração).
    unsigned char probe[sizeof(kIcon2x2)];
    std::memcpy(probe, kIcon2x2, sizeof(probe));
    if (!w.set_window_icon(probe, 2, 2)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [2] set_window_icon(valid 2x2) "
                   "returned false\n");
      ++failures;
    }
    if (std::memcmp(probe, kIcon2x2, sizeof(probe)) != 0) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [2] caller's buffer was mutated by "
                   "set_window_icon()\n");
      ++failures;
    }
    std::puts(
        "app_window_icon_sanity [2] WindowGlfw::set_window_icon() contract OK "
        "(cannot verify a visible icon under Xvfb -- no window manager)");
  }

  // ---------------------------------------------------------------------------
  // [3] App forward-coverage -- same accepted path, reached through the App facade.
  // ---------------------------------------------------------------------------
  {
    glintfx::AppConfig cfg;
    cfg.title = "app_window_icon_sanity_app";
    cfg.width = 320;
    cfg.height = 240;
    glintfx::App app(cfg);
    if (!app.ok()) {
      std::puts("SKIP: App init failed -- no display / GL context");
      return failures; // EN: report any [1]/[2] failures already found. PT: reporta falhas de [1]/[2] já achadas.
    }

    if (app.set_window_icon(nullptr, 2, 2)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [3] App::set_window_icon(nullptr, ...) "
                   "returned true -- forward to WindowGlfw's null guard broken?\n");
      ++failures;
    }
    if (!app.set_window_icon(kIcon2x2, 2, 2)) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [3] App::set_window_icon(valid 2x2) "
                   "returned false\n");
      ++failures;
    }

    // EN: Confirm the pipeline stays live after the call (same proof pattern as
    //     app_vsync_sanity.cpp's step [3]).
    // PT: Confirma que o pipeline continua ativo após a chamada (mesmo padrão de prova do
    //     passo [3] de app_vsync_sanity.cpp).
    app.poll_events();
    app.update();
    app.render();
    if (!app.ok()) {
      std::fprintf(stderr,
                   "app_window_icon_sanity FAIL: [3] App::ok() false after render() "
                   "following set_window_icon()\n");
      ++failures;
    }
  }

  if (failures == 0) {
    std::puts("app_window_icon_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "app_window_icon_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
