// SPDX-License-Identifier: MPL-2.0
// EN: Integration test for LOGTHR-1 (Onda 2, framework-2D --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decision D8, section 4.2). Loads
//     log_throttle_scene.rml (a document with NO font-family anywhere, its label text bound to
//     a data-model number -- see that file's own header) into an embed UiLayer, then mutates
//     the bound counter and update()+render()s it 100 times -- the SAME per-frame shape a real
//     game loop with a live HUD counter uses (a STATIC label would format once and never dirty
//     again, which would NOT reproduce the reported flood; empirically confirmed while writing
//     this test). RmlUi core logs "No font face defined. Missing 'font-family' property. On
//     element ..." (ElementText.cpp:86, upstream, pinned) once per re-formatted layout pass
//     while that condition holds; pre-LOGTHR-1 this was ~100+ raw lines on stderr (the flood the
//     consumer reported). Captures stderr for the whole loop (freopen to a file, restore via
//     dup2 afterwards) and asserts the LINE COUNT stays small -- proving the dedup/throttle
//     override (src/system_clock.hpp's SystemClock::LogMessage) is actually wired into the
//     embed SystemInterface path, not just unit-tested in isolation (log_dedup_sanity.cpp).
//
//     Runs in BOTH GLINTFX_BACKEND_GLFW build configs (registered via ${_embed_dep} in
//     tests/CMakeLists.txt, same pattern as ui_layer_attach/ui_layer_sanity above it) -- proving
//     the fix reaches the ONLY real consumer's build shape (GusWorld, embed-only, GLFW=OFF).
//
//     POSIX-only (dup/dup2/fileno for the stderr swap): safe here because this .cpp is a TEST
//     binary, never compiled by the MSVC job (that job runs with GLINTFX_BUILD_TESTS=OFF, see
//     .github/workflows/ci.yml's windows-msvc-build step) -- only src/log_dedup.hpp and
//     src/system_clock.hpp (part of the glintfx library target itself) need to stay portable.
// PT: Teste de integração do LOGTHR-1 (Onda 2, framework-2D --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decisão D8, seção 4.2). Carrega
//     log_throttle_scene.rml (documento SEM font-family em lugar nenhum, ver o próprio
//     cabeçalho daquele arquivo) num UiLayer embed, depois faz update()+render() 100 vezes -- o
//     MESMO formato por-frame que um loop de jogo real usa. O núcleo do RmlUi loga "No font
//     face defined. Missing 'font-family' property. On element ..." (ElementText.cpp:86,
//     upstream, pinado) uma vez por passe de layout enquanto essa condição persistir; antes do
//     LOGTHR-1 isso eram ~100+ linhas cruas no stderr (o flood que o consumidor reportou).
//     Captura o stderr do loop inteiro (freopen para um arquivo, restaura via dup2 depois) e
//     verifica que a CONTAGEM DE LINHAS fica pequena -- provando que o override de
//     dedup/throttle (SystemClock::LogMessage de src/system_clock.hpp) está de fato ligado ao
//     caminho de SystemInterface do embed, não só testado isolado em unidade
//     (log_dedup_sanity.cpp).
//
//     Roda nas DUAS configs de build de GLINTFX_BACKEND_GLFW (registrado via ${_embed_dep} em
//     tests/CMakeLists.txt, mesmo padrão de ui_layer_attach/ui_layer_sanity acima dele) --
//     provando que o fix alcança o formato de build do ÚNICO consumidor real (GusWorld,
//     embed-only, GLFW=OFF).
//
//     Só-POSIX (dup/dup2/fileno pra troca de stderr): seguro aqui porque este .cpp é um
//     BINÁRIO DE TESTE, nunca compilado pelo job MSVC (aquele job roda com
//     GLINTFX_BUILD_TESTS=OFF, ver o passo windows-msvc-build de .github/workflows/ci.yml) --
//     só src/log_dedup.hpp e src/system_clock.hpp (parte do próprio alvo da lib glintfx)
//     precisam continuar portáveis.
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace {

// EN: Counts '\n' bytes in the file at `path` -- a plain, dependency-free line counter (the
//     capture file is always small: at most a few hundred bytes in this test).
// PT: Conta bytes '\n' no arquivo em `path` -- um contador de linhas simples, sem dependência
//     (o arquivo de captura é sempre pequeno: no máximo algumas centenas de bytes neste teste).
long count_lines(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return -1;
  std::ostringstream buf;
  buf << in.rdbuf();
  const std::string content = buf.str();
  long lines = 0;
  for (char c : content)
    if (c == '\n') ++lines;
  return lines;
}

} // namespace

int main() {
  // ---------------------------------------------------------------------------
  // EN: Host creates a hidden window (GL context provider) — same fixture shape as every other
  //     embed test in this suite (ui_layer_attach, ui_layer_sanity, ...).
  // PT: Host cria janela oculta (provê o contexto GL) — mesmo fixture dos outros testes embed
  //     desta suíte (ui_layer_attach, ui_layer_sanity, ...).
  // ---------------------------------------------------------------------------
  glintfx::WindowGlfw host;
  if (!host.create("log_throttle_host", 320, 240)) {
    std::puts("log_throttle_embed_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = 320, .logical_height = 240, .load_gl = true});
  if (!ui.ok()) {
    std::puts("log_throttle_embed_sanity FAIL: ui attach failed");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // EN: Redirect stderr to a capture file BEFORE load()/the frame loop -- the flood (and its
  //     throttle) is what we are measuring, not our own diagnostics (which go to stdout).
  //     dup() the original fd first so it can be restored afterwards -- freopen() alone cannot
  //     be reversed back to the process's real stderr.
  // PT: Redireciona stderr para um arquivo de captura ANTES do load()/loop de frames -- o flood
  //     (e o throttle dele) é o que estamos medindo, não nossos próprios diagnósticos (que vão
  //     pro stdout). dup() do fd original primeiro para poder restaurar depois -- freopen()
  //     sozinho não pode ser revertido de volta ao stderr real do processo.
  // ---------------------------------------------------------------------------
  const char* capture_path = "log_throttle_capture.tmp";
  std::fflush(stderr);
  const int saved_stderr_fd = dup(fileno(stderr));
  if (saved_stderr_fd < 0 || !freopen(capture_path, "w", stderr)) {
    std::puts("log_throttle_embed_sanity FAIL: could not redirect stderr for capture");
    return 3;
  }

  // EN: The bound counter must be created/bound BEFORE load() (RmlUi data-view compilation
  //     happens at parse time -- same ordering constraint as every other data-model test in
  //     this suite, e.g. data_model_embed_sanity.cpp).
  // PT: O contador ligado precisa ser criado/ligado ANTES do load() (a compilação de data-view
  //     do RmlUi acontece em tempo de parse -- mesma restrição de ordem de todo outro teste de
  //     data-model desta suíte, ex.: data_model_embed_sanity.cpp).
  ui.create_data_model("flood");
  ui.bind_number("frame_counter", 0.0);
  ui.load("log_throttle_scene.rml");

  // EN: 100 frames -- the exact shape section 4.2 of the plan asks for ("a font-family-less
  //     label rendered 100 frames"), mirroring a real per-frame game loop with a live HUD
  //     counter: set_number() dirties the bound text node every iteration, so
  //     LogMissingFontFace fires on every re-formatted layout pass, not just once.
  // PT: 100 frames -- a forma exata que a seção 4.2 do plano pede ("um label sem font-family
  //     renderizado 100 frames"), espelhando um loop de jogo real com um contador de HUD vivo:
  //     set_number() suja o nó de texto ligado a cada iteração, então LogMissingFontFace
  //     dispara a cada passe de layout re-formatado, não só uma vez.
  for (int i = 0; i < 100; ++i) {
    ui.set_number("frame_counter", static_cast<double>(i));
    ui.update();
    ui.render();
  }

  // ---------------------------------------------------------------------------
  // EN: Restore the real stderr (dup2 back over fd 2) before reporting PASS/FAIL -- from here
  //     on, fprintf(stderr, ...) reaches the terminal/CI log again, not the capture file.
  // PT: Restaura o stderr real (dup2 de volta sobre o fd 2) antes de reportar PASS/FAIL -- daqui
  //     em diante, fprintf(stderr, ...) alcança o terminal/log de CI de novo, não o arquivo de
  //     captura.
  // ---------------------------------------------------------------------------
  std::fflush(stderr);
  dup2(saved_stderr_fd, fileno(stderr));
  close(saved_stderr_fd);

  const long lines = count_lines(capture_path);
  std::remove(capture_path);

  if (lines < 0) {
    std::puts("log_throttle_embed_sanity FAIL: could not read back the stderr capture file");
    return 4;
  }

  std::printf("log_throttle_embed_sanity: %ld line(s) on stderr across 100 frames\n", lines);

  // EN: Small constant, generously above the handful of lines the D8 policy actually produces
  //     (1st occurrence + a summary at count 10 + a summary at count 100, at most a few of
  //     those if update()+render() each trigger their own occurrence) -- but two orders of
  //     magnitude below the ~100+ raw lines the SAME 100-frame loop produced before this slice.
  // PT: Constante pequena, generosamente acima do punhado de linhas que a política do D8
  //     realmente produz (1ª ocorrência + sumário na contagem 10 + sumário na contagem 100, no
  //     máximo algumas dessas se update()+render() cada um disparar sua própria ocorrência) --
  //     mas duas ordens de grandeza abaixo das ~100+ linhas cruas que o MESMO loop de 100
  //     frames produzia antes desta fatia.
  constexpr long kMaxAcceptableLines = 10;
  if (lines > kMaxAcceptableLines) {
    std::printf("log_throttle_embed_sanity FAIL: %ld lines > %ld (throttle not effective)\n",
                lines, kMaxAcceptableLines);
    return 5;
  }

  std::puts("log_throttle_embed_sanity: PASS");
  return 0;
}
